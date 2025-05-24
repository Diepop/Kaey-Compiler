cmake_minimum_required(VERSION 3.5)
project("Kaey Compiler")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(SYSTEM ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/)

if (WIN32)
    add_compile_definitions(NOMINMAX)
    add_compile_options(/external:anglebrackets /external:env:INCLUDE /external:I${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include /external:W0 /bigobj)
    if (CMAKE_BUILD_TYPE MATCHES DEBUG)
        add_compile_options(/JMC)
    endif()
else()
    # GCC or Clang
endif()

add_subdirectory("PCH")

add_subdirectory("Kaey")

add_executable(Compiler "Source.cpp")
target_link_libraries(Compiler PUBLIC Kaey)

target_precompile_headers(Compiler REUSE_FROM Kaey)

add_executable(KaeyTest "Test.cpp")
target_link_libraries(KaeyTest PUBLIC KaeyLlvm)

target_precompile_headers(KaeyTest REUSE_FROM Kaey)

set(OpenCV_ROOT "${VCPKG_INSTALLED_DIR}/x64-windows-static-md/share/opencv2")
find_package(OpenCV REQUIRED)
#find_package(OpenCV CONFIG REQUIRED)
add_executable(ParserTest "ParserTest.cpp" "Kaey/Others/GraphvizPlotter.cpp")
target_link_libraries(ParserTest PUBLIC
    opencv_highgui
    KaeyLexer KaeyParser
)

target_precompile_headers(ParserTest REUSE_FROM Kaey)
