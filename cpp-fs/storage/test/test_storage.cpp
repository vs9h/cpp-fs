#include <gtest/gtest.h>

#include "error_types.h"
#include "partition/in_memory_partition.hpp"
#include "storage.hpp"

namespace tests::storage {

namespace {
template <typename T>
cppfs::storage::Storage<T>* CreateDefaultStorage() {
  return new cppfs::storage::Storage<T>(std::make_unique<T>());
}

template <typename T>
class StorageTest : public testing::Test {
 protected:
  void SetUp() final { storage_.reset(CreateDefaultStorage<T>()); }

  void TearDown() final { storage_->Clear(); }

  std::unique_ptr<cppfs::storage::Storage<T>> storage_;
};

constexpr auto kValidUUID = "a2c59f5c-6c9b-4800-afb8-282fc5e743cc";
constexpr auto kInvalidUUID = "00000000-0000-0000-0000-000000000000";
}  // namespace

using PartitionManagers =
    testing::Types<cppfs::storage::InMemoryPartitionManager>;

TYPED_TEST_SUITE(StorageTest, PartitionManagers);

TYPED_TEST(StorageTest, CreatePartition) {
  std::string uuid{kValidUUID};
  auto partition = this->storage_->CreatePartition(uuid);
  ASSERT_TRUE(partition.has_value() && partition.value() != nullptr);

  auto has_partition = this->storage_->LookupPartition(uuid).has_value();
  ASSERT_TRUE(has_partition);
}

TYPED_TEST(StorageTest, CreatePartitionIncorrectUUID) {
  auto partition = this->storage_->CreatePartition(kInvalidUUID);
  ASSERT_FALSE(partition.has_value());
  ASSERT_EQ(partition.error().code, cppfs::storage::ErrorEnum::kInvalidInput);
}

TYPED_TEST(StorageTest, LookupPartitionIncorrectUUID) {
  auto partition = this->storage_->LookupPartition(kInvalidUUID);
  ASSERT_FALSE(partition.has_value());
  ASSERT_EQ(partition.error().code, cppfs::storage::ErrorEnum::kInvalidInput);
}

TYPED_TEST(StorageTest, CreatePartitionAlreadyExists) {
  std::string uuid{kValidUUID};
  auto partition = this->storage_->CreatePartition(uuid);
  ASSERT_TRUE(partition.has_value());
  partition = this->storage_->CreatePartition(uuid);
  ASSERT_EQ(partition.error().code, cppfs::storage::ErrorEnum::kAlreadyExists);
}

}  // namespace tests::storage
