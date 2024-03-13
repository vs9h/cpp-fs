#pragma once

#include <cassert>
#include <string>
#include <tl/expected.hpp>
#include <unordered_map>
#include <vector>

#include "error_types.h"
#include "partition.hpp"

namespace cppfs::storage {

class InMemoryPartition final : public Partition {
 public:
  explicit InMemoryPartition(size_t kb) : data_(kb << 10) {}

 private:
  std::vector<char> data_;
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
