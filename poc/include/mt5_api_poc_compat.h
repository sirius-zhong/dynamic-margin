#pragma once

#if !defined(POC_USE_STUB_MT5_API) && __has_include("MT5APIServer.h")
#include "MT5APIServer.h"
#else

#include <cstdint>

using MTAPIRES = unsigned int;
using UINT = unsigned int;
using UINT64 = std::uint64_t;
using LPCWSTR = const wchar_t*;

constexpr MTAPIRES MT_RET_OK = 0;
constexpr MTAPIRES MT_RET_ERROR = 1;
constexpr MTAPIRES MT_RET_ERR_PARAMS = 10013;

struct MTPluginInfo {
    UINT version = 0;
    UINT api_version = 0;
    LPCWSTR name = nullptr;
    LPCWSTR copyright = nullptr;
    LPCWSTR description = nullptr;
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
    virtual ~IMTServerPlugin() = default;
    virtual void Release() = 0;
    virtual MTAPIRES Start(IMTServerAPI* server) = 0;
    virtual MTAPIRES Stop() = 0;
};

struct IMTTradeSink {
    virtual ~IMTTradeSink() = default;

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
};

struct IMTServerAPI {
    virtual MTAPIRES TradeSubscribe(IMTTradeSink*) { return MT_RET_OK; }
    virtual MTAPIRES TradeUnsubscribe(IMTTradeSink*) { return MT_RET_OK; }
};

struct IMTConGateway {};
struct IMTConGroup {};
struct IMTConSymbol {};
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
    virtual UINT DealReason() const { return 0; }
    virtual UINT64 Position() const { return 0; }
    virtual UINT64 PositionBy() const { return 0; }
    virtual ~IMTExecution() = default;
};

struct IMTOrder {
    virtual UINT64 Login() const { return 0; }
    virtual LPCWSTR Symbol() const { return L""; }
    virtual UINT64 Order() const { return 0; }
    virtual UINT Type() const { return 0; }
    virtual UINT Reason() const { return 0; }
    virtual double RateMargin() const { return 1.0; }
    virtual MTAPIRES RateMargin(const double) { return MT_RET_OK; }
    virtual ~IMTOrder() = default;
};

struct IMTDeal {
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
    virtual UINT Entry() const { return ENTRY_IN; }
    virtual UINT Reason() const { return 0; }
    virtual double RateMargin() const { return 1.0; }
    virtual MTAPIRES RateMargin(const double) { return MT_RET_OK; }
    virtual ~IMTDeal() = default;
};

struct IMTPosition {
    virtual UINT64 Login() const { return 0; }
    virtual LPCWSTR Symbol() const { return L""; }
    virtual UINT64 Position() const { return 0; }
    virtual UINT Action() const { return 0; }
    virtual double RateMargin() const { return 1.0; }
    virtual MTAPIRES RateMargin(const double) { return MT_RET_OK; }
    virtual ~IMTPosition() = default;
};

#endif
