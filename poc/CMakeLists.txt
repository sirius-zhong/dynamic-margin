cmake_minimum_required(VERSION 3.20)

project(MinuteParityMarginPoc LANGUAGES CXX)

option(POC_USE_STUB_MT5_API "Build with the local minimal MT5 API stub for syntax checks only." OFF)
option(POC_SET_ORDER_RATE_MARGIN "Apply odd-minute factor to IMTOrder::RateMargin." ON)
option(POC_SET_DEAL_RATE_MARGIN "Apply odd-minute factor to IMTDeal::RateMargin." ON)
option(POC_SET_POSITION_RATE_MARGIN "Apply odd-minute factor to IMTPosition::RateMargin." ON)

set(MT5_SERVER_API_INCLUDE_DIR "" CACHE PATH "Directory containing MT5APIServer.h")
set(MT5_SERVER_API_LIBRARY "" CACHE FILEPATH "Optional MT5 Server API import library")

add_library(minute_parity_margin_poc SHARED
    src/minute_parity_margin_poc.cpp
)

target_compile_features(minute_parity_margin_poc PRIVATE cxx_std_17)

target_include_directories(minute_parity_margin_poc PRIVATE
    include
)

if(MT5_SERVER_API_INCLUDE_DIR)
    target_include_directories(minute_parity_margin_poc PRIVATE "${MT5_SERVER_API_INCLUDE_DIR}")
endif()

if(POC_USE_STUB_MT5_API)
    target_compile_definitions(minute_parity_margin_poc PRIVATE POC_USE_STUB_MT5_API=1)
endif()

target_compile_definitions(minute_parity_margin_poc PRIVATE
    POC_SET_ORDER_RATE_MARGIN=$<BOOL:${POC_SET_ORDER_RATE_MARGIN}>
    POC_SET_DEAL_RATE_MARGIN=$<BOOL:${POC_SET_DEAL_RATE_MARGIN}>
    POC_SET_POSITION_RATE_MARGIN=$<BOOL:${POC_SET_POSITION_RATE_MARGIN}>
)

if(MT5_SERVER_API_LIBRARY)
    target_link_libraries(minute_parity_margin_poc PRIVATE "${MT5_SERVER_API_LIBRARY}")
endif()

if(MSVC)
    target_compile_options(minute_parity_margin_poc PRIVATE /W4 /permissive-)
else()
    target_compile_options(minute_parity_margin_poc PRIVATE -Wall -Wextra -Wpedantic)
endif()
