#pragma once

#include <string>

namespace cppfs::storage {

enum class ErrorEnum {
  kInvalidInput,
  kNotFound,
  kAlreadyExists,
  kOutOfMemory,
  kDirectory,
  kInternalServerError,
};

struct Error {
  ErrorEnum code;
  std::string message;
};

}  // namespace cppfs::storage
