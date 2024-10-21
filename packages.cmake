include(FetchContent)

cmake_policy(PUSH)
cmake_policy(SET CMP0150 NEW)

FetchContent_Declare(cpp-postgres
    GIT_REPOSITORY ../cpp-postgres.git
    GIT_TAG        4edc6eeb2af2fdd0f148cd814f2481546dc49689 # 0.4.0
)

FetchContent_Declare(verp
    GIT_REPOSITORY ../verp.git
    GIT_TAG        ba52d9213b8b31fac463cd3b44c8821f6e15457f # 0.2.0
)

FetchContent_MakeAvailable(cpp-postgres verp)

cmake_policy(POP)
