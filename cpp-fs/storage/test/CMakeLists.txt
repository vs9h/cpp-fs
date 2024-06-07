project(storage-tests CXX)

add_executable(${PROJECT_NAME} test_partition.cpp test_storage.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE storagelib gtest::gtest)

add_test(NAME ${PROJECT_NAME} COMMAND $<TARGET_FILE:${PROJECT_NAME}>)
