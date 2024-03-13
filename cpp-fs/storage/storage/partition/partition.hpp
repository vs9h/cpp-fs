#pragma once

#include <string>
#include <tl/expected.hpp>

#include "error_types.h"

namespace cppfs::storage {

class Partition {
 public:
  virtual ~Partition() = default;
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
      std::string const& uuid, size_t kb) = 0;

  virtual void DestroyPartition(std::string const& uuid) = 0;
  /// clear all partition manager data
  virtual void Clear() = 0;
};

};  // namespace cppfs::storage
