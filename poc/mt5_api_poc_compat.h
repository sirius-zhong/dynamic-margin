#pragma once

#if defined(POC_USE_STUB_MT5_API)
#define POC_USING_STUB_MT5_API 1
#elif __has_include("MT5APIServer.h")
#include "MT5APIServer.h"
#elif __has_include("../MetaTrader5SDK/Include/MT5APIServer.h")
#include "../MetaTrader5SDK/Include/MT5APIServer.h"
#else
#error "Cannot find MT5APIServer.h. Add MetaTrader5SDK/Include to the include path, or define POC_USE_STUB_MT5_API only for local smoke tests."
#endif

#if defined(POC_USING_STUB_MT5_API)

#include <cstdint>

using MTAPIRES = unsigned int;
using UINT = unsigned int;
using UINT64 = std::uint64_t;
using LPCWSTR = const wchar_t*;

#ifdef _WIN32
#define MTAPIENTRY extern "C" __declspec(dllexport)
#else
#define MTAPIENTRY extern "C" __attribute__((visibility("default")))
#endif

constexpr MTAPIRES MT_RET_OK = 0;
constexpr MTAPIRES MT_RET_OK_NONE = 1;
constexpr MTAPIRES MT_RET_ERROR = 2;
constexpr MTAPIRES MT_RET_ERR_PARAMS = 3;
constexpr MTAPIRES MT_RET_ERR_MEM = 6;

constexpr UINT MTLogOK = 0;
constexpr UINT MTLogErr = 2;

struct MTPluginInfo {
    UINT version = 0;
    UINT version_api = 0;
    wchar_t name[64] = {};
    wchar_t copyright[128] = {};
    wchar_t description[256] = {};
    UINT defaults_total = 0;
    UINT reserved[128] = {};
};

struct MTServerInfo {
    wchar_t platform_name[64] = {};
    wchar_t platform_owner[128] = {};
    UINT server_version = 0;
    UINT server_build = 0;
    UINT server_type = 0;
    UINT64 server_id = 0;
    UINT reserved[32] = {};
};

struct IMTServerAPI;
struct IMTRequest;
struct IMTConfirm;
struct IMTConGroup;
struct IMTConSymbol;
struct IMTPosition;
struct IMTOrder;
struct IMTDeal;
struct IMTExecution;
struct IMTConGateway;

struct IMTServerPlugin {
    virtual void Release() = 0;
    virtual MTAPIRES Start(IMTServerAPI* server) = 0;
    virtual MTAPIRES Stop() = 0;
};

struct IMTTradeSink {
    virtual MTAPIRES HookTradeRequestProcess(
        const IMTRequest*,
        const IMTConfirm*,
        const IMTConGroup*,
        const IMTConSymbol*,
        IMTPosition*,
        IMTOrder*,
        IMTDeal*) {
        return MT_RET_OK;
    }

    virtual MTAPIRES HookTradeRequestProcessCloseBy(
        const IMTRequest*,
        const IMTConfirm*,
        const IMTConGroup*,
        const IMTConSymbol*,
        IMTPosition*,
        IMTOrder*,
        IMTDeal*,
        IMTDeal*) {
        return MT_RET_OK;
    }

    virtual MTAPIRES HookTradeExecution(
        const IMTConGateway*,
        const IMTExecution*,
        const IMTConGroup*,
        const IMTConSymbol*,
        IMTPosition*,
        IMTOrder*,
        IMTDeal*) {
        return MT_RET_OK;
    }

    virtual void OnTradeExecution(
        const IMTConGateway*,
        const IMTExecution*,
        const IMTConGroup*,
        const IMTConSymbol*,
        const IMTPosition*,
        const IMTOrder*,
        const IMTDeal*) {
    }
};

struct IMTServerAPI {
    virtual MTAPIRES About(MTServerInfo&) { return MT_RET_OK; }
    virtual MTAPIRES LoggerOut(const UINT, LPCWSTR, ...) { return MT_RET_OK; }
    virtual MTAPIRES LoggerOutString(const UINT, LPCWSTR) { return MT_RET_OK; }
    virtual MTAPIRES TradeSubscribe(IMTTradeSink*) { return MT_RET_OK; }
    virtual MTAPIRES TradeUnsubscribe(IMTTradeSink*) { return MT_RET_OK; }
};

struct IMTConGateway {};
struct IMTConGroup {};
struct IMTConSymbol {
    enum EnMarginRateTypes : UINT {
        MARGIN_RATE_BUY = 0,
        MARGIN_RATE_SELL = 1,
        MARGIN_RATE_BUY_LIMIT = 2,
        MARGIN_RATE_SELL_LIMIT = 3,
        MARGIN_RATE_BUY_STOP = 4,
        MARGIN_RATE_SELL_STOP = 5,
        MARGIN_RATE_BUY_STOP_LIMIT = 6,
        MARGIN_RATE_SELL_STOP_LIMIT = 7,
        MARGIN_RATE_FIRST = MARGIN_RATE_BUY,
        MARGIN_RATE_LAST = MARGIN_RATE_SELL_STOP_LIMIT,
    };

    virtual LPCWSTR Symbol() const { return L""; }
    virtual double MarginRateMaintenance(const UINT) const { return 1.0; }
    virtual MTAPIRES MarginRateMaintenance(const UINT, const double) { return MT_RET_OK; }
    virtual ~IMTConSymbol() = default;
};
struct IMTConfirm {};

struct IMTRequest {
    virtual UINT64 Login() const { return 0; }
    virtual LPCWSTR Symbol() const { return L""; }
    virtual UINT Action() const { return 0; }
    virtual UINT Reason() const { return 0; }
    virtual UINT64 Position() const { return 0; }
    virtual UINT64 PositionBy() const { return 0; }
    virtual ~IMTRequest() = default;
};

struct IMTExecution {
    virtual UINT64 Login() const { return 0; }
    virtual LPCWSTR Symbol() const { return L""; }
    virtual UINT Action() const { return 0; }
    virtual UINT OrderType() const { return 0; }
    virtual UINT DealReason() const { return 0; }
    virtual UINT64 Position() const { return 0; }
    virtual UINT64 PositionBy() const { return 0; }
    virtual ~IMTExecution() = default;
};

struct IMTOrder {
    enum EnOrderType : UINT {
        OP_BUY = 0,
        OP_SELL = 1,
        OP_BUY_LIMIT = 2,
        OP_SELL_LIMIT = 3,
        OP_BUY_STOP = 4,
        OP_SELL_STOP = 5,
        OP_BUY_STOP_LIMIT = 6,
        OP_SELL_STOP_LIMIT = 7,
        OP_CLOSE_BY = 8,
        OP_FIRST = OP_BUY,
        OP_LAST = OP_CLOSE_BY,
    };

    virtual UINT64 Login() const { return 0; }
    virtual LPCWSTR Symbol() const { return L""; }
    virtual UINT64 Order() const { return 0; }
    virtual UINT Type() const { return 0; }
    virtual UINT Reason() const { return 0; }
    virtual ~IMTOrder() = default;
};

struct IMTDeal {
    enum EnDealAction : UINT {
        DEAL_BUY = 0,
        DEAL_SELL = 1,
    };

    enum EnDealEntry : UINT {
        ENTRY_IN = 0,
        ENTRY_OUT = 1,
        ENTRY_INOUT = 2,
        ENTRY_OUT_BY = 3,
    };

    virtual UINT64 Login() const { return 0; }
    virtual LPCWSTR Symbol() const { return L""; }
    virtual UINT64 Deal() const { return 0; }
    virtual UINT64 Order() const { return 0; }
    virtual UINT64 PositionID() const { return 0; }
    virtual UINT Action() const { return DEAL_BUY; }
    virtual UINT Entry() const { return ENTRY_IN; }
    virtual UINT Reason() const { return 0; }
    virtual ~IMTDeal() = default;
};

struct IMTPosition {
    enum EnPositionAction : UINT {
        POSITION_BUY = 0,
        POSITION_SELL = 1,
    };

    virtual UINT64 Login() const { return 0; }
    virtual LPCWSTR Symbol() const { return L""; }
    virtual UINT64 Position() const { return 0; }
    virtual UINT Action() const { return 0; }
    virtual ~IMTPosition() = default;
};

constexpr UINT MTServerAPIVersion = 5830;

#endif
