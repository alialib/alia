include(FetchContent)

FetchContent_Declare(cereal
    GIT_REPOSITORY https://github.com/USCiLab/cereal.git
    GIT_TAG v1.3.0
    GIT_SHALLOW TRUE
)

if(NOT cereal_POPULATED)
    FetchContent_Populate(cereal)
endif()
