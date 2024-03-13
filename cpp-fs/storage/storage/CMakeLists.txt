project(storage CXX)

set(STORAGE_NAME storagelib)
add_library(${STORAGE_NAME} storage.cpp)
target_link_libraries(${STORAGE_NAME} PRIVATE Boost::headers)
target_link_libraries(${STORAGE_NAME} PUBLIC tl::expected)
target_include_directories(${STORAGE_NAME}
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${STORAGE_NAME})
