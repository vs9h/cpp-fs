#pragma once

#include <cassert>
#include <fstream>
#include <string>
#include <tl/expected.hpp>
#include <unistd.h>
#include <vector>

#include "error_types.h"
#include "partition.hpp"

namespace cppfs::storage {

class OnDiskRegularFile : public RegularFile {
 public:
  explicit OnDiskRegularFile(std::filesystem::path path)
      : file_path_(std::move(path)) {}

  size_t GetSize() const override {
    return std::filesystem::file_size(file_path_);
  }

  ssize_t Seek(size_t offset) override {
    if (offset >= GetSize()) {
      return -1;
    }

    offset_ = offset;
    return 0;
  }

  ssize_t Read(std::ostream& out, size_t nbytes) override {
    std::ifstream file(file_path_, std::ios::binary);
    if (!file) return -1;

    file.seekg(static_cast<std::streamoff>(offset_));
    std::vector<char> buffer(nbytes);
    file.read(buffer.data(), static_cast<std::streamsize>(nbytes));

    out.write(buffer.data(), file.gcount());
    offset_ += static_cast<size_t>(file.gcount());
    return file.gcount();
  }

  ssize_t PositionalRead(std::ostream& out, size_t offset,
                         size_t nbytes) override {
    std::ifstream file(file_path_, std::ios::binary);
    if (!file) return -1;

    file.seekg(static_cast<std::streamoff>(offset));
    std::vector<char> buffer(nbytes);
    file.read(buffer.data(), static_cast<std::streamoff>(nbytes));

    out.write(buffer.data(), file.gcount());
    return file.gcount();
  }

 private:
  std::filesystem::path file_path_;
  size_t offset_{0};
};

class OnDiskDirectory : public Directory {
 public:
  explicit OnDiskDirectory(std::filesystem::path path)
      : dir_path_(std::move(path)) {}

  tl::expected<RegularFile*, Error> StoreRegularFile(
      std::string const& name, std::string&& data) override {
    std::filesystem::path file_path = dir_path_ / name;
    std::ofstream file(file_path, std::ios::binary);
    if (!file) {
      return tl::unexpected(
          Error{ErrorEnum::kInternalServerError,
                std::format("Failed to create file '{}'", name)});
    }

    file << data;
    file.close();

    files_.push_back(std::make_unique<OnDiskRegularFile>(file_path));
    return files_.back().get();
  }

  tl::expected<Directory*, Error> CreateDirectory(
      std::string const& name) override {
    std::filesystem::path new_dir_path = dir_path_ / name;

    if (!std::filesystem::create_directory(new_dir_path)) {
      return tl::unexpected(
          Error{ErrorEnum::kAlreadyExists,
                std::format("Cannot create directory '{}'", name)});
    }

    directories_.push_back(std::make_unique<OnDiskDirectory>(new_dir_path));
    return directories_.back().get();
  }

  std::vector<DirEntry> GetDirEntries() const {
    std::vector<DirEntry> entries;
    for (auto const& entry : std::filesystem::directory_iterator(dir_path_)) {
      std::string name = entry.path().filename().string();
      FileType type =
          entry.is_directory() ? FileType::Directory : FileType::Regular;
      size_t size =
          entry.is_directory() ? 0 : std::filesystem::file_size(entry.path());
      entries.push_back({name, type, size});
    }
    return entries;
  }

  size_t GetSize() const override {
    size_t size = 0;
    for (auto const& entry : std::filesystem::directory_iterator(dir_path_)) {
      size += entry.path().filename().string().size() + sizeof(entry);
    }
    return size;
  }

 private:
  std::filesystem::path dir_path_;
  std::vector<std::unique_ptr<OnDiskDirectory>> directories_;
  std::vector<std::unique_ptr<OnDiskRegularFile>> files_;
};

class OnDiskPartition final : public Partition {
 public:
  explicit OnDiskPartition(std::filesystem::path partition_path)
      : partition_path_(std::move(partition_path)), root_(partition_path_) {
    std::filesystem::create_directories(partition_path_);
  }

  tl::expected<File*, Error> Open(std::filesystem::path const& path) override {
    if (path.is_absolute()) {
      return Open(&root_, path.relative_path());
    }
    return tl::unexpected(
        Error{ErrorEnum::kNotFound,
              std::format("Expected absolute path, but received '{}'",
                          path.string())});
  }

  tl::expected<File*, Error> Open(Directory* base_dir,
                                  std::filesystem::path const& path) override {
    auto dir = static_cast<OnDiskDirectory*>(base_dir);
    if (path.empty()) {
      return dir;
    }

    std::filesystem::path full_path = partition_path_ / path;

    if (std::filesystem::is_directory(full_path)) {
      directories_.push_back(std::make_unique<OnDiskDirectory>(full_path));
      return directories_.back().get();
    } else if (std::filesystem::is_regular_file(full_path)) {
      files_.push_back(std::make_unique<OnDiskRegularFile>(full_path));
      return files_.back().get();
    } else {
      return tl::unexpected(
          Error{ErrorEnum::kNotFound,
                std::format("File '{}' not found", path.string())});
    }
  }

 private:
  std::filesystem::path partition_path_;
  std::vector<std::unique_ptr<OnDiskDirectory>> directories_;
  std::vector<std::unique_ptr<OnDiskRegularFile>> files_;
  OnDiskDirectory root_;
};

class OnDiskPartitionManager final : public PartitionManager {
 public:
  bool ContainsPartition(std::string const& uuid) const final {
    return std::filesystem::exists(root_path_ / uuid);
  }

  Partition* LookupPartition(std::string const& uuid) final {
    if (ContainsPartition(uuid)) {
      return &partitions_.try_emplace(uuid, root_path_ / uuid).first->second;
    }
    return nullptr;
  }

  tl::expected<Partition*, Error> CreatePartition(
      std::string const& uuid) final {
    std::filesystem::path partition_path = root_path_ / uuid;
    if (std::filesystem::create_directories(partition_path)) {
      auto [it, _] = partitions_.try_emplace(uuid, partition_path);
      return &it->second;
    }
    return tl::unexpected(
        Error{ErrorEnum::kInternalServerError,
              std::format("Failed to create partition for UUID '{}'", uuid)});
  }

  void DestroyPartition(std::string const& uuid) final {
    std::filesystem::remove_all(root_path_ / uuid);
    partitions_.erase(uuid);
  }

  void Clear() final {
    std::filesystem::remove_all(root_path_);
    partitions_.clear();
  }

 private:
  std::filesystem::path root_path_{
      "./partitions"};
  std::unordered_map<std::string, OnDiskPartition> partitions_;
};

};  // namespace cppfs::storage
