#include <cstdlib>
#include <iostream>

#include <boost/json.hpp>

#include "error_types.h"
#include "httplib.h"
#include "partition/in_memory_partition.hpp"
#include "partition/partition.hpp"
#include "storage.hpp"

namespace cppfs::storage {

namespace {
template <typename T>
cppfs::storage::Storage<T>* CreateDefaultStorage() {
  return new cppfs::storage::Storage<T>(std::make_unique<T>());
}

constexpr auto kValidUUID = "a2c59f5c-6c9b-4800-afb8-282fc5e743cc";

httplib::StatusCode ErrorEnumToStatusCode(ErrorEnum internal_code) {
  using namespace cppfs::storage;
  switch (internal_code) {
    case ErrorEnum::kInvalidInput:
      return httplib::NotFound_404;
    case ErrorEnum::kNotFound:
      return httplib::NotFound_404;
    case ErrorEnum::kAlreadyExists:
      return httplib::BadRequest_400;
    case ErrorEnum::kOutOfMemory:
      return httplib::NotFound_404;
    case ErrorEnum::kDirectory:
      return httplib::Forbidden_403;
    default:
      return httplib::InternalServerError_500;
  }
}

/// Set error as response
/// Override status code if code is provided.
void SetError(
    httplib::Response& res, Error const& error,
    std::optional<httplib::StatusCode> status_code_opt = std::nullopt) {
  httplib::StatusCode status =
      status_code_opt.value_or(ErrorEnumToStatusCode(error.code));
  boost::json::object error_info = {
      {"message", error.message},
      {"internal_status", static_cast<int>(error.code)},
      {"status", status},
  };
  res.status = status;
  res.set_content(boost::json::serialize(error_info), "text/json");
}

}  // namespace

void SetCorsHeaders(httplib::Response& res) {
  res.set_header("Access-Control-Allow-Origin", "*");
  res.set_header("Access-Control-Allow-Methods",
                 "GET, POST, PUT, DELETE, OPTIONS");
  res.set_header(
      "Access-Control-Allow-Headers",
      "Origin, X-Requested-With, Content-Type, Accept, Authorization");
}

int StartFS(std::string const& host, int port) {
  /* Init some test data */
  std::string uuid{kValidUUID};
  auto storage =
      CreateDefaultStorage<cppfs::storage::InMemoryPartitionManager>();
  auto partition = storage->CreatePartition(uuid);
  cppfs::storage::Directory* root = partition.value()->OpenRoot();
  root->StoreRegularFile("16B.txt", "012345678901234");

  httplib::Server server;
  if (!server.is_valid()) {
    std::cout << "Cannot start storage server" << std::endl;
    return EXIT_FAILURE;
  }

  server.Options("/(.*)", [&](httplib::Request const& req [[maybe_unused]],
                              httplib::Response& res) { SetCorsHeaders(res); });

  server.set_post_routing_handler(
      [](httplib::Request const& req [[maybe_unused]], httplib::Response& res) {
        SetCorsHeaders(res);
      });

  server.Get("/ping", [](httplib::Request const& req [[maybe_unused]],
                         httplib::Response& res) {
    res.set_content("pong\n", "text/plain");
  });

  server.Get("/partition/:id", [&](httplib::Request const& req,
                                   httplib::Response& res) {
    std::string const& partition_id = req.path_params.at("id");

    tl::expected<Partition*, Error> partition_expected =
        storage->LookupPartition(partition_id);

    if (!partition_expected.has_value()) {
      SetError(res, partition_expected.error());
      return;
    }

    boost::json::object partition_info = {
        {"uuid", partition_id},
        {"is_valid", true},
    };

    res.set_content(boost::json::serialize(partition_info), "application/json");
  });

  server.Get("/ls", [&](httplib::Request const& req, httplib::Response& res) {
    std::filesystem::path path{req.get_param_value("path")};
    if (path.empty()) path = "/";

    tl::expected<Directory*, Error> dir_expected =
        partition.value()->OpenDir(path);
    if (!dir_expected.has_value()) {
      SetError(res, dir_expected.error());
      return;
    }

    Directory* dir = dir_expected.value();
    boost::json::array direntries;
    for (Directory::DirEntry const& direntry : dir->GetDirEntries())
      direntries.push_back(
          boost::json::value{{"name", direntry.name},
                             {"size", direntry.size},
                             {"type_id", static_cast<int>(direntry.type)},
                             {"type", FileTypeToString(direntry.type)}});

    boost::json::object ls_res{
        {"entries", std::move(direntries)},
        {"name", path.filename().c_str()},
        {"size", dir->GetSize()},
        {"type_id", static_cast<int>(dir->GetType())},
        {"type", FileTypeToString(dir->GetType())},
    };

    res.set_content(boost::json::serialize(ls_res), "application/json");
  });

  server.Get("/cat", [&](httplib::Request const& req, httplib::Response& res) {
    std::filesystem::path path{req.get_param_value("path")};

    tl::expected<RegularFile*, Error> reg_file_expected =
        partition.value()->OpenRegularFile(path);
    if (!reg_file_expected.has_value()) {
      SetError(res, reg_file_expected.error());
      return;
    }

    RegularFile* reg_file = reg_file_expected.value();
    std::stringstream ss;
    std::string offset_str =
        req.has_param("offset") ? req.get_param_value("offset") : "";
    std::size_t offset{offset_str.empty() ? 0 : std::stoull(offset_str)};
    if (offset > reg_file->GetSize()) {
      SetError(
          res,
          Error{.code = ErrorEnum::kInternalServerError,
                .message = std::format(
                    "Received incorrect offset (offset: {}, file size: {})",
                    offset, reg_file->GetSize())});
      return;
    }

    std::string size_str =
        req.has_param("size") ? req.get_param_value("size") : "";
    std::size_t size =
        size_str.empty() ? reg_file->GetSize() - offset : std::stoull(size_str);
    if (offset + size > reg_file->GetSize()) {
      SetError(res, Error{.code = ErrorEnum::kInternalServerError,
                          .message = std::format(
                              "Received incorrect read size (offset: {}, size: "
                              "{}, file size: {})",
                              offset, size, reg_file->GetSize())});
      return;
    }

    ssize_t read_bytes = reg_file->PositionalRead(ss, offset, size);
    if (read_bytes == -1) {
      SetError(res,
               Error{.code = ErrorEnum::kInternalServerError,
                     .message = std::format(
                         "Cannot read {} bytes from offset {} from file {}",
                         size, offset, path.c_str())});
      return;
    }

    res.set_content(ss.str(), "application/text");
  });

  server.Post(
      "/mkdir", [&](httplib::Request const& req, httplib::Response& res) {
        std::filesystem::path dir_path{req.get_param_value("at")};
        if (dir_path.empty()) dir_path = "/";

        std::string file_name{req.get_param_value("dir")};
        if (file_name.empty()) {
          SetError(res,
                   Error{.code = ErrorEnum::kInvalidInput,
                         .message = std::format("File name wasn't specified")});
          return;
        }

        tl::expected<Directory*, Error> dir_expected =
            partition.value()->OpenDir(dir_path);
        if (!dir_expected.has_value()) {
          SetError(res, dir_expected.error());
          return;
        }

        Directory* dir = dir_expected.value();
        tl::expected<Directory*, Error> new_dir_expected =
            dir->CreateDirectory(file_name);
        if (!new_dir_expected.has_value()) {
          SetError(res, new_dir_expected.error());
          return;
        }

        Directory* new_dir = new_dir_expected.value();
        boost::json::object mkdir_res{
            {"dir", dir_path.c_str()},
            {"name", file_name.c_str()},
            {"size", new_dir->GetSize()},
            {"type_id", static_cast<int>(new_dir->GetType())},
            {"type", FileTypeToString(new_dir->GetType())},
        };
        res.set_content(boost::json::serialize(mkdir_res), "application/json");
      });

  server.Post("/store", [&](httplib::Request const& req,
                            httplib::Response& res) {
    std::filesystem::path dir_path{req.get_param_value("dir")};
    if (dir_path.empty()) dir_path = "/";

    std::string file_name{req.get_param_value("file")};
    if (file_name.empty()) {
      SetError(res,
               Error{.code = ErrorEnum::kInvalidInput,
                     .message = std::format("File name wasn't specified")});
      return;
    }

    std::string data{req.get_param_value("data")};
    if (data.empty()) {
      SetError(res, Error{.code = ErrorEnum::kInvalidInput,
                          .message = std::format("Cannot store empty file {}",
                                                 file_name.c_str())});
      return;
    }

    tl::expected<Directory*, Error> dir_expected =
        partition.value()->OpenDir(dir_path);
    if (!dir_expected.has_value()) {
      SetError(res, dir_expected.error());
      return;
    }

    Directory* dir = dir_expected.value();
    tl::expected<RegularFile*, Error> reg_file_expected =
        dir->StoreRegularFile(file_name, std::move(data));
    if (!reg_file_expected.has_value()) {
      SetError(res, reg_file_expected.error());
      return;
    }
    RegularFile* reg_file = reg_file_expected.value();
    boost::json::object store_res{
        {"dir", dir_path.c_str()},
        {"name", file_name.c_str()},
        {"size", reg_file->GetSize()},
        {"type_id", static_cast<int>(reg_file->GetType())},
        {"type", FileTypeToString(reg_file->GetType())},
    };
    res.set_content(boost::json::serialize(store_res), "application/json");
  });

  server.listen(host, port);
  return 0;
}

}  // namespace cppfs::storage
