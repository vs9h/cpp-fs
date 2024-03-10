project(balancer CXX)

set(BALANCER_NAME balancerlib)
add_library(${BALANCER_NAME} balancer.cpp)
target_link_libraries(${BALANCER_NAME} PRIVATE Boost::headers)
target_include_directories(${BALANCER_NAME}
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
  )

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${BALANCER_NAME})
