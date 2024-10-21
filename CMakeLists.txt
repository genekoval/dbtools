cmake_minimum_required(VERSION 3.27)

include(cmake/modules.cmake)
include(GitVersion)

project(dbtools
    VERSION ${GIT_VERSION_ABBREV}
    DESCRIPTION "C++ library for managing database data"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS NO)

include(packages.cmake)

add_library(dbtools)
add_library(${PROJECT_NAME}::dbtools ALIAS dbtools)
target_sources(dbtools PUBLIC FILE_SET HEADERS BASE_DIRS include)
target_link_libraries(dbtools
    PUBLIC
        cpp-postgres::pg++
        verp::verp
)

add_subdirectory(include)
add_subdirectory(src)

include(Sanitizers)
enable_asan()

include(EasyInstall)
easy_install(
    TARGETS
        dbtools
    DEPENDENCIES
        cpp-postgres
        verp
)

if(PROJECT_IS_TOP_LEVEL)
    add_custom_target(docker
        docker build --progress=plain .
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        USES_TERMINAL
    )
endif()
