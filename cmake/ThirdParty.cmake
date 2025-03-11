include(FetchContent)

FetchContent_Declare(
  libassert
  URL https://github.com/jeremy-rifkin/libassert/archive/refs/tags/v2.0.2.tar.gz
  URL_HASH MD5=73221c3af3bafdd8552c059d9bab5fbd
)

FetchContent_MakeAvailable(libassert)

FetchContent_Declare(
  Catch2
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.6.0.tar.gz
  URL_HASH MD5=86a9fec7afecaec687faaa988ac6818e
)

if (${CMAKE_BUILD_TYPE} MATCHES ".*TSAN.*")
  # Disable signal handling in Catch2, as it floods with unnecessary messages in thread-sanitizer.
  # https://github.com/catchorg/Catch2/issues/1833
  add_definitions(-DCATCH_CONFIG_NO_POSIX_SIGNALS)
endif()

FetchContent_MakeAvailable(Catch2)


find_package(tl-expected REQUIRED)
