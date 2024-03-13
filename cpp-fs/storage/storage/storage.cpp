#include "storage.hpp"

#include <stdexcept>

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace cppfs::storage {

/// This function is a modification of the function from
/// https://stackoverflow.com/a/28685177
bool IsUUIDValid(std::string const& str) noexcept {
  using namespace boost::uuids;
  try {
    return string_generator()(str).version() != uuid::version_unknown;
  } catch (std::runtime_error const& error [[maybe_unused]]) {
    return false;
  }
}

}  // namespace cppfs::storage
