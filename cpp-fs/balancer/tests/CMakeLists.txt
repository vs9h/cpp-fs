project(balancer-tests CXX)

add_executable(${PROJECT_NAME} test_balancer.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE balancerlib gtest::gtest)

add_test(NAME ${PROJECT_NAME} COMMAND $<TARGET_FILE:${PROJECT_NAME}>)
