project(generator-${LIBROMFS_PROJECT_NAME})
set(CMAKE_CXX_STANDARD 20)

# This is a code generator, so when cross compiling it should be built with the host (native) toolchain
if(CMAKE_CROSSCOMPILING)
    SET(CMAKE_C_COMPILER ${NATIVE_CMAKE_C_COMPILER})
    SET(CMAKE_CXX_COMPILER ${NATIVE_CMAKE_CXX_COMPILER})
    unset(CMAKE_C_OSX_DEPLOYMENT_TARGET_FLAG)
    unset(CMAKE_CXX_OSX_DEPLOYMENT_TARGET_FLAG)
    SET(CMAKE_C_FLAGS ${NATIVE_CMAKE_C_FLAGS})
    SET(CMAKE_CXX_FLAGS ${NATIVE_CMAKE_CXX_FLAGS})
endif()


# Add sources
add_executable(${PROJECT_NAME}
    source/main.cpp
)

string(REPLACE ";" "," LIBROMFS_RESOURCE_LOCATION_STRING "${LIBROMFS_RESOURCE_LOCATION}")

target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCE_LOCATION="${LIBROMFS_RESOURCE_LOCATION_STRING}" LIBROMFS_PROJECT_NAME="${LIBROMFS_PROJECT_NAME}")
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Export generated romfs resource file to libromfs
set(ROMFS "libromfs_resources.cpp" PARENT_SCOPE)
set(ROMFS "libromfs_resources.cpp")