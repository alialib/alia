include(FetchContent)

FetchContent_Declare(scnlib
  GIT_REPOSITORY
  https://github.com/eliaskosunen/scnlib
  GIT_TAG v0.4
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(scnlib)
