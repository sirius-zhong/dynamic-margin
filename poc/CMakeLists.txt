cmake_minimum_required(VERSION 3.20)

project(SymbolMaintenanceMarginPoc LANGUAGES CXX)

option(POC_USE_STUB_MT5_API "Build with the local minimal MT5 API stub for syntax checks only." OFF)
option(POC_REQUIRE_OPENING_DEAL "Only modify symbol rate when deal exists and deal entry is ENTRY_IN." ON)

set(MT5_SERVER_API_INCLUDE_DIR "" CACHE PATH "Directory containing MT5APIServer.h")
set(MT5_SERVER_API_LIBRARY "" CACHE FILEPATH "Optional MT5 Server API import library")

add_library(symbol_maintenance_margin_poc SHARED
    stdafx.cpp
    SymbolMaintenanceMargin.cpp
    PluginInstance.cpp
)

target_compile_features(symbol_maintenance_margin_poc PRIVATE cxx_std_17)

target_include_directories(symbol_maintenance_margin_poc PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(MT5_SERVER_API_INCLUDE_DIR)
    target_include_directories(symbol_maintenance_margin_poc PRIVATE "${MT5_SERVER_API_INCLUDE_DIR}")
endif()

if(POC_USE_STUB_MT5_API)
    target_compile_definitions(symbol_maintenance_margin_poc PRIVATE POC_USE_STUB_MT5_API=1)
endif()

target_compile_definitions(symbol_maintenance_margin_poc PRIVATE
    POC_REQUIRE_OPENING_DEAL=$<BOOL:${POC_REQUIRE_OPENING_DEAL}>
)

if(MT5_SERVER_API_LIBRARY)
    target_link_libraries(symbol_maintenance_margin_poc PRIVATE "${MT5_SERVER_API_LIBRARY}")
endif()

if(MSVC)
    target_compile_options(symbol_maintenance_margin_poc PRIVATE /W4 /permissive-)
else()
    target_compile_options(symbol_maintenance_margin_poc PRIVATE -Wall -Wextra -Wpedantic)
endif()
