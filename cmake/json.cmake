include(FetchContent)

message(STATUS "Fetching nlohmann/json")

FetchContent_Declare(json
  GIT_REPOSITORY
  https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
  GIT_TAG v3.9.1
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(json)
