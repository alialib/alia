message(STATUS "Fetching alia-html")
include(FetchContent)
FetchContent_Declare(alia-html
  GIT_REPOSITORY https://github.com/tmadden/alia-html
  GIT_TAG main
)
FetchContent_MakeAvailable(alia-html)
