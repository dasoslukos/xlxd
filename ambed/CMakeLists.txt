cmake_minimum_required(VERSION 3.20)

project(ambed VERSION 1.3.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB AMBED_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(ambed ${AMBED_SOURCES})

if(MSVC)
    # Statically link the Microsoft C/C++ runtime so the Windows release
    # does not require the Visual C++ Redistributable to be installed.
    set_property(TARGET ambed PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

target_include_directories(ambed PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(WIN32)
    target_compile_definitions(ambed PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )

    set(D2XX_LIBRARY
        "${CMAKE_CURRENT_SOURCE_DIR}/../build-support/d2xx/ftd2xx.lib"
        CACHE FILEPATH
        "Path to the x64 FTDI D2XX import library"
    )

    if(NOT EXISTS "${D2XX_LIBRARY}")
        message(FATAL_ERROR
            "FTDI D2XX import library not found: ${D2XX_LIBRARY}\n"
            "Set -DD2XX_LIBRARY=<path-to-ftd2xx.lib>"
        )
    endif()

    target_link_libraries(ambed PRIVATE
        "${D2XX_LIBRARY}"
        ws2_32
        winmm
        advapi32
    )
else()
    find_package(Threads REQUIRED)

    set(D2XX_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/ftdi/d2xx/1.4.6/x86_64"
        CACHE PATH
        "FTDI D2XX runtime root"
    )

    set(D2XX_LIBRARY
        "${D2XX_ROOT}/lib/libftd2xx.so.1.4.6"
        CACHE FILEPATH
        "Path to the FTDI D2XX library"
    )

    if(NOT EXISTS "${D2XX_LIBRARY}")
        message(FATAL_ERROR
            "FTDI D2XX library not found: ${D2XX_LIBRARY}\n"
            "Set -DD2XX_LIBRARY=<path-to-libftd2xx>"
        )
    endif()

    target_compile_options(ambed PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    target_link_libraries(ambed PRIVATE
        "${D2XX_LIBRARY}"
        Threads::Threads
    )
endif()
