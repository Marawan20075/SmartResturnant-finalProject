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

option(SMART_RESTAURANT_BUILD_TESTS "Build the database integration tests" OFF)
if(SMART_RESTAURANT_BUILD_TESTS)
    add_executable(DatabaseTests tests/database_tests.cpp)
    target_include_directories(DatabaseTests PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(DatabaseTests PRIVATE libpqxx::pqxx)
endif()
