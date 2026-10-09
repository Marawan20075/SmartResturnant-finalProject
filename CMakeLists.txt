cmake_minimum_required(VERSION 3.16)
project(SmartRestaurant VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(libpqxx CONFIG REQUIRED)

add_executable(SmartRestaurant
    main.cpp
    mainwindow.cpp
    mainwindow.h
    loginpage.cpp
    loginpage.h
    customerpage.cpp
    customerpage.h
    staffpage.cpp
    staffpage.h
)

target_include_directories(SmartRestaurant PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(SmartRestaurant PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    libpqxx::pqxx
)

# libpqxx from MSYS2/vcpkg is usually built with a newer MinGW than the Qt kit.
# Without its own runtime DLLs next to the exe, Windows loads the kit's older
# libwinpthread/libstdc++ from PATH and startup fails with e.g.
# "The procedure entry point nanosleep64 could not be located".
set(SMART_RESTAURANT_RUNTIME_DLLS "")
if(MINGW)
    foreach(_prop IMPORTED_LOCATION IMPORTED_LOCATION_RELEASE
                  IMPORTED_LOCATION_RELWITHDEBINFO IMPORTED_LOCATION_DEBUG)
        get_target_property(_pqxx_dll libpqxx::pqxx ${_prop})
        if(_pqxx_dll)
            break()
        endif()
    endforeach()
    if(_pqxx_dll)
        get_filename_component(_pqxx_bin "${_pqxx_dll}" DIRECTORY)
        foreach(_dll libpqxx.dll libpq.dll
                     libwinpthread-1.dll libstdc++-6.dll libgcc_s_seh-1.dll
                     libssl-3-x64.dll libcrypto-3-x64.dll libintl-8.dll libiconv-2.dll)
            if(EXISTS "${_pqxx_bin}/${_dll}")
                list(APPEND SMART_RESTAURANT_RUNTIME_DLLS "${_pqxx_bin}/${_dll}")
            endif()
        endforeach()
        message(STATUS "Copying libpqxx runtime DLLs from ${_pqxx_bin}")
    endif()
endif()

function(copy_runtime_dlls target)
    if(SMART_RESTAURANT_RUNTIME_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${SMART_RESTAURANT_RUNTIME_DLLS} $<TARGET_FILE_DIR:${target}>
            VERBATIM)
    endif()
endfunction()

copy_runtime_dlls(SmartRestaurant)

option(SMART_RESTAURANT_BUILD_TESTS "Build the database integration tests" OFF)
if(SMART_RESTAURANT_BUILD_TESTS)
    add_executable(DatabaseTests tests/database_tests.cpp)
    target_include_directories(DatabaseTests PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(DatabaseTests PRIVATE libpqxx::pqxx)
    copy_runtime_dlls(DatabaseTests)
endif()
