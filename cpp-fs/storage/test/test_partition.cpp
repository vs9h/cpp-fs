#include <gtest/gtest.h>
#include <sstream>

#include "error_types.h"
#include "partition/in_memory_partition.hpp"
#include "partition/partition.hpp"
#include "storage.hpp"

namespace tests::storage {

namespace {
template <typename T>
cppfs::storage::Storage<T>* CreateDefaultStorage() {
  return new cppfs::storage::Storage<T>(std::make_unique<T>());
}

using namespace cppfs::storage;

constexpr auto kValidUUID = "a2c59f5c-6c9b-4800-afb8-282fc5e743cc";

template <typename T>
class PartitionTest : public testing::Test {
 protected:
  void SetUp() final {
    storage_.reset(CreateDefaultStorage<T>());

    auto partition_expected = storage_->CreatePartition(kValidUUID);
    ASSERT_TRUE(partition_expected.has_value());

    partition_ = partition_expected.value();
  }

  void TearDown() final { storage_->Clear(); }

  std::unique_ptr<Storage<T>> storage_;
  Partition* partition_;
};

constexpr bool operator==(Directory::DirEntry const& lhs,
                          Directory::DirEntry const& rhs) {
  auto create_tuple = [](Directory::DirEntry const& dir_entry) {
    return std::tie(dir_entry.name, dir_entry.size, dir_entry.type);
  };
  return create_tuple(lhs) == create_tuple(rhs);
}
}  // namespace

using PartitionManagers = testing::Types<InMemoryPartitionManager>;

TYPED_TEST_SUITE(PartitionTest, PartitionManagers);

TYPED_TEST(PartitionTest, GetRootDirectory) {
  Directory* root = this->partition_->OpenRoot();
  ASSERT_TRUE(root != nullptr);
  ASSERT_TRUE(root->GetDirEntries().empty());
}

TYPED_TEST(PartitionTest, StoreRegularFile) {
  Directory* root = this->partition_->OpenRoot();

  auto constexpr file_data = "012345678901234";

  Directory::DirEntry expected_direntry = {
      .name = "16B.txt",
      .type = FileType::Regular,
      .size = 16,
  };

  auto reg_file_expected =
      root->StoreRegularFile(expected_direntry.name, file_data);
  ASSERT_TRUE(reg_file_expected.has_value());
  std::vector<Directory::DirEntry> const& actual_direntries =
      root->GetDirEntries();
  ASSERT_EQ(actual_direntries.size(), 1);
  Directory::DirEntry const& actual_direntry = actual_direntries.front();
  ASSERT_TRUE(expected_direntry == actual_direntry);

  std::stringstream ss;
  ssize_t rc = reg_file_expected.value()->Read(ss, expected_direntry.size);
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(ss.str(), file_data);
}

TYPED_TEST(PartitionTest, StoreRegularFileAlreadyExists) {
  Directory* root = this->partition_->OpenRoot();

  auto constexpr file_name = "16B.txt";
  auto constexpr file_data = "012345678901234";

  auto reg_file_expected = root->StoreRegularFile(file_name, file_data);
  ASSERT_TRUE(reg_file_expected.has_value());
  reg_file_expected = root->StoreRegularFile(file_name, file_data);
  ASSERT_FALSE(reg_file_expected.has_value());
  ASSERT_EQ(reg_file_expected.error().code, ErrorEnum::kAlreadyExists);
}

TYPED_TEST(PartitionTest, CreateDirectory) {
  Directory* root = this->partition_->OpenRoot();

  auto constexpr dir_name = "tmp";

  auto dir_expected = root->CreateDirectory(dir_name);
  ASSERT_TRUE(dir_expected.has_value());
  Directory* dir = dir_expected.value();
  ASSERT_EQ(dir->GetDirEntries().size(), 0);
  auto file_expected = this->partition_->Open(std::string{"/"} + dir_name);
  ASSERT_TRUE(file_expected.has_value());
}

}  // namespace tests::storage
