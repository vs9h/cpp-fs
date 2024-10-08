#pragma once

#include <cassert>
#include <filesystem>
#include <format>
#include <ostream>
#include <shared_mutex>
#include <string>
#include <tl/expected.hpp>
#include <unistd.h>

#include "error_types.h"

namespace cppfs::storage {

/// Supported file types
enum class FileType {
  Regular,
  Directory,
};

inline std::string FileTypeToString(FileType type) {
  switch (type) {
    case FileType::Regular:
      return "Regular";
    case FileType::Directory:
      return "Directory";
  }
  return "Unknown";
}

class File {
 public:
  using Offset = ssize_t;

  explicit File(FileType type) : type_(type) {};
  virtual ~File() = default;

  virtual size_t GetSize() const = 0;

  FileType GetType() const { return type_; }

 private:
  FileType type_;
};

class RegularFile : public File {
 public:
  RegularFile() : File(FileType::Regular) {}

  virtual ssize_t Read(std::ostream& out, size_t nbytes) = 0;
  /* Works like lseek() with `SEEK_SET` flag */
  virtual ssize_t Seek(size_t offset) = 0;
  virtual ssize_t PositionalRead(std::ostream& out, size_t offset,
                                 size_t nbytes) = 0;
};

class Directory : public File {
 public:
  struct DirEntry {
    std::string name;
    FileType type;
    size_t size;
  };

  Directory() : File(FileType::Directory) {}

  virtual tl::expected<Directory*, Error> CreateDirectory(
      std::string const& name) = 0;
  virtual std::vector<DirEntry> GetDirEntries() const = 0;
  virtual tl::expected<RegularFile*, Error> StoreRegularFile(
      std::string const& name, std::string&& data) = 0;
};

class Partition {
 public:
  virtual ~Partition() = default;

  /// open file relative to the partition root
  virtual tl::expected<File*, Error> Open(
      std::filesystem::path const& path) = 0;

  /// open file relative to @c dir
  virtual tl::expected<File*, Error> Open(
      Directory* dir, std::filesystem::path const& path) = 0;

  /// open directory relative to the partition root
  virtual tl::expected<Directory*, Error> OpenDir(
      std::filesystem::path const& path) {
    tl::expected<File*, Error> file_expected = Open(path);
    if (!file_expected.has_value())
      return tl::unexpected{file_expected.error()};

    File* file = file_expected.value();
    auto dir = dynamic_cast<Directory*>(file);
    if (dir == nullptr) {
      return tl::unexpected(Error{
          ErrorEnum::kDirectory,
          std::format("Expected directory, but received regular file '{}'",
                      path.filename().c_str())});
    }
    return dir;
  }

  /// open regular file relative to the partition root
  virtual tl::expected<RegularFile*, Error> OpenRegularFile(
      std::filesystem::path const& path) {
    tl::expected<File*, Error> file_expected = Open(path);
    if (!file_expected.has_value())
      return tl::unexpected{file_expected.error()};

    File* file = file_expected.value();
    auto reg_file = dynamic_cast<RegularFile*>(file);
    if (reg_file == nullptr) {
      return tl::unexpected(Error{
          ErrorEnum::kDirectory,
          std::format("Expected regular file, but received directory '{}'",
                      path.filename().c_str())});
    }
    return reg_file;
  }

  Directory* OpenRoot() {
    auto root_file = Open("/");
    assert(root_file.has_value());
    return static_cast<Directory*>(root_file.value());
  }

 private:
  mutable std::shared_mutex mutex_;
};

class PartitionManager {
 public:
  virtual ~PartitionManager() = default;

  /// check if partition with specified uuid exists
  virtual bool ContainsPartition(std::string const& uuid) const = 0;

  /// return partition if found, nullptr otherwise
  virtual Partition* LookupPartition(std::string const& uuid) = 0;
  /// try to create new partition
  virtual tl::expected<Partition*, Error> CreatePartition(
      std::string const& uuid) = 0;

  virtual void DestroyPartition(std::string const& uuid) = 0;
  /// clear all partition manager data
  virtual void Clear() = 0;
};

};  // namespace cppfs::storage
