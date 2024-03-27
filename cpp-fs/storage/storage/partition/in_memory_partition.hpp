#pragma once

#include <algorithm>
#include <cassert>
#include <format>
#include <numeric>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "error_types.h"
#include "partition.hpp"

namespace cppfs::storage {

class InMemoryRegularFile : public RegularFile {
 public:
  explicit InMemoryRegularFile(std::string_view view) : view_(view) {}

  size_t GetSize() const override { return view_.size(); }

  ssize_t Seek(size_t offset) override {
    if (offset >= view_.size()) {
      return -1;
    }

    offset_ = offset;
    return 0;
  }

  ssize_t Read(std::ostream& out, size_t nbytes) override {
    if (offset_ + nbytes > view_.size()) {
      return -1;
    }

    out << view_.substr(offset_, nbytes);
    return 0;
  }

  ssize_t PositionalRead(std::ostream& out, size_t offset,
                         size_t nbytes) override {
    if (offset + nbytes > view_.size()) {
      return -1;
    }
    out << view_.substr(offset, nbytes);
    return 0;
  }

 private:
  std::string_view view_;
  size_t offset_{};
};

class InMemoryDirectory : public Directory {
 public:
  explicit InMemoryDirectory() = default;

  tl::expected<void, Error> StoreRegularFile(std::string const& name,
                                             std::string_view data) {
    assert(!entries_.contains(name));
    entries_.emplace(name, std::make_unique<InMemoryRegularFile>(data));
    return {};
  }

  std::vector<DirEntry> GetDirEntries() const override {
    std::vector<DirEntry> entries;
    entries.reserve(entries_.size());
    std::transform(entries_.cbegin(), entries_.cend(),
                   std::back_inserter(entries), [](auto const& entry) {
                     auto const& [name, file] = entry;
                     return DirEntry{name, file->GetType(), file->GetSize()};
                   });
    return entries;
  }

  std::unordered_map<std::string, std::unique_ptr<File>> const&
  GetInMemoryEntries() const {
    return entries_;
  };

  size_t GetSize() const override {
    return std::accumulate(entries_.cbegin(), entries_.cend(), 0UL,
                           [](size_t acc, auto const& entry) {
                             auto const& [name, file] = entry;
                             return acc + name.capacity() + sizeof(file);
                           });
  }

 private:
  std::unordered_map<std::string, std::unique_ptr<File>> entries_{};
};

class InMemoryPartition final : public Partition {
 public:
  explicit InMemoryPartition(size_t kb) : data_(kb << 10) {}

  tl::expected<File*, Error> Open(
      std::filesystem::path const& path) const override {
    if (*path.begin() == "\"/\"") {
      return Open(&root_, path.relative_path());
    }
    return tl::unexpected(Error{
        ErrorEnum::kNotFound,
        std::format("Expected absolute path, but received {}", path.string())});
  }

  /// open file relative to @c dir
  tl::expected<File*, Error> Open(
      Directory const* base_dir,
      std::filesystem::path const& path) const override {
    auto dir = static_cast<InMemoryDirectory const*>(base_dir);
    for (auto it = path.begin(); it != path.end(); ++it) {
      for (auto const& [name, file] : dir->GetInMemoryEntries()) {
        if (name != *it) continue;
        if (it == path.end()) return file.get();
        if (file->GetType() == FileType::Regular) {
          return tl::unexpected(Error{
              ErrorEnum::kNotFound,
              std::format("Expected directory, but received regular file '{}'",
                          name)});
        }
        dir = static_cast<InMemoryDirectory const*>(file.get());
        break;
      }
    }
    return tl::unexpected(
        Error{ErrorEnum::kNotFound,
              std::format("File '{}' not found", path.string())});
  }

 private:
  std::vector<char> data_;
  InMemoryDirectory root_;
};

class InMemoryPartitionManager final : public PartitionManager {
 public:
  bool ContainsPartition(std::string const& uuid) const final {
    return partitions_.contains(uuid);
  }

  Partition* LookupPartition(std::string const& uuid) final {
    auto it = partitions_.find(uuid);
    return it == partitions_.end() ? nullptr : &it->second;
  }

  tl::expected<Partition*, Error> CreatePartition(std::string const& uuid,
                                                  size_t kb) final {
    assert(!ContainsPartition(uuid));
    auto [it, _] = partitions_.try_emplace(uuid, kb);
    return &it->second;
  }

  void DestroyPartition(std::string const& uuid) final {
    partitions_.erase(uuid);
  }

  void Clear() final { partitions_.clear(); }

 private:
  std::unordered_map<std::string, InMemoryPartition> partitions_;
};

};  // namespace cppfs::storage
