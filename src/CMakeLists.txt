# Bot Application

set(APP_NAME update-vcpkg-port)

set(SOURCE_FILES
    "command.h"
    "command.cpp"
    "config.h"
    "config.cpp"
    "main.cpp"
    "manifest.h"
    "manifest.cpp"
    "portfile.h"
    "portfile.cpp"
    "updater.h"
    "updater.cpp"
    "utils.h"
    "utils.cpp"
)

add_executable(${APP_NAME} ${SOURCE_FILES})

target_compile_features(${APP_NAME} PRIVATE cxx_std_20) # Enforce C++20 or newer
set_target_properties(${APP_NAME} PROPERTIES CXX_EXTENSIONS off)

if (MSVC)
    # Force MSVC to use utf-8 encoding regardless of whether the BOM exists
    target_compile_options(${APP_NAME} PRIVATE "/utf-8")
endif ()

# Warnings and errors settings
# Use highest reasonable warning level, and treat warnings as errors
if (MSVC) # Visual Studio
    if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]") # If default /W3 presents
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}") # Change /W3 to /W4
    else ()
        target_compile_options(${APP_NAME} PRIVATE /W4) # Add /W4 directly
    endif ()
    target_compile_options(${APP_NAME} PRIVATE /WX) # Treat warnings as errors
    # Treat all header files specified by angle brackets to be system headers, and ignore all those warnings
    target_compile_options(${APP_NAME} PRIVATE 
        /experimental:external /external:W0 /external:anglebrackets)
else () # Not Visual Studio, assuming gcc or clang
    target_compile_options(${APP_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif ()

find_package(argh CONFIG REQUIRED)
find_package(Boost COMPONENTS filesystem REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_include_directories(${APP_NAME} PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(${APP_NAME} PRIVATE
    argh
    ${Boost_LIBRARIES}
    fmt::fmt
    nlohmann_json::nlohmann_json
)
