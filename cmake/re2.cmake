include(FetchContent)

set(RE2_BUILD_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  re2
  GIT_REPOSITORY https://github.com/google/re2.git
  GIT_TAG 2022-06-01
)
FetchContent_MakeAvailable(re2)
