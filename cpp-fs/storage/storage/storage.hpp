#pragma once

#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <tl/expected.hpp>
#include <type_traits>

#include "error_types.h"
#include "partition/partition.hpp"

namespace cppfs::storage {

/// check if provided string is valid uuid
bool IsUUIDValid(std::string const& str) noexcept;

///
/// The main class that provides an interface for interacting with the storage.
///
template <typename Manager>
class Storage {
 public:
  static_assert(std::is_base_of_v<PartitionManager, Manager>,
                "Manager Type should be derived from PartitionManager");

  explicit Storage(std::unique_ptr<Manager> manager)
      : manager_(std::move(manager)) {}

  /// try to find partition by partition uuid
  tl::expected<Partition*, Error> LookupPartition(std::string const& uuid) {
    if (!IsUUIDValid(uuid)) {
      return tl::unexpected(
          Error{ErrorEnum::kInvalidInput,
                std::format("Received incorrect partition id '{}'", uuid)});
    }
    Partition* const partition = manager_->LookupPartition(uuid);
    if (partition == nullptr) {
      return tl::unexpected(
          Error{ErrorEnum::kNotFound,
                std::format("Partition with id '{}' not found", uuid)});
    }

    return partition;
  }

  /// try to create new partition
  tl::expected<Partition*, Error> CreatePartition(std::string const& uuid,
                                                  size_t kb) {
    auto const lookup = LookupPartition(uuid);
    if (lookup) {
      return tl::unexpected(
          Error{ErrorEnum::kAlreadyExists,
                std::format("Partition with id '{}' already exists", uuid)});
    }
    if (lookup.error().code != ErrorEnum::kNotFound) {
      return tl::unexpected(lookup.error());
    }
    std::lock_guard const lock_guard{mutex_};
    return manager_->CreatePartition(uuid, kb);
  }

  void Clear() { manager_->Clear(); }

 private:
  std::unique_ptr<Manager> const manager_;
  std::mutex mutex_;
};

};  // namespace cppfs::storage
