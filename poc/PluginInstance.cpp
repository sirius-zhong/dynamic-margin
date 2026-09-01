#include "stdafx.h"
#include "PluginInstance.h"

#include <cmath>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#ifndef POC_REQUIRE_OPENING_DEAL
#define POC_REQUIRE_OPENING_DEAL 1
#endif

namespace {

constexpr uint64_t kTargetLogin = 15031;
constexpr double kOddMinuteFactor = 2.0;
constexpr double kUnsetRateBase = 1.0;
constexpr const wchar_t* kPluginName = L"SymbolMaintenanceMargin.PoC";

std::mutex g_log_mutex;

std::tm LocalServerTime() {
    std::time_t now = std::time(nullptr);
    std::tm result{};
#ifdef _WIN32
    localtime_s(&result, &now);
#else
    localtime_r(&now, &result);
#endif
    return result;
}

std::wstring NowText(const std::tm& tm) {
    std::wstringstream out;
    out << std::put_time(&tm, L"%Y-%m-%d %H:%M:%S");
    return out.str();
}

std::wstring SafeText(LPCWSTR value) {
    return value == nullptr ? L"" : value;
}

void LogLine(IMTServerAPI* api, const uint32_t level, const std::wstring& line) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    const std::tm tm = LocalServerTime();

    std::wofstream file("symbol_maintenance_margin_poc.log", std::ios::app);
    file << NowText(tm) << L" " << line << L"\n";

    if (api != nullptr) {
        api->LoggerOut(level, L"%s", line.c_str());
    }

#ifdef _WIN32
    const std::wstring debug = line + L"\n";
    OutputDebugStringW(debug.c_str());
#endif
}

uint64_t ResolveLogin(
    const IMTExecution* execution,
    const IMTPosition* position,
    const IMTOrder* order,
    const IMTDeal* deal) {
    if (deal != nullptr && deal->Login() != 0) {
        return deal->Login();
    }
    if (position != nullptr && position->Login() != 0) {
        return position->Login();
    }
    if (order != nullptr && order->Login() != 0) {
        return order->Login();
    }
    if (execution != nullptr && execution->Login() != 0) {
        return execution->Login();
    }
    return 0;
}

std::wstring ResolveSymbol(
    const IMTExecution* execution,
    const IMTPosition* position,
    const IMTOrder* order,
    const IMTDeal* deal,
    const IMTConSymbol* symbol) {
    if (deal != nullptr && deal->Symbol() != nullptr && deal->Symbol()[0] != L'\0') {
        return SafeText(deal->Symbol());
    }
    if (position != nullptr && position->Symbol() != nullptr && position->Symbol()[0] != L'\0') {
        return SafeText(position->Symbol());
    }
    if (order != nullptr && order->Symbol() != nullptr && order->Symbol()[0] != L'\0') {
        return SafeText(order->Symbol());
    }
    if (execution != nullptr && execution->Symbol() != nullptr && execution->Symbol()[0] != L'\0') {
        return SafeText(execution->Symbol());
    }
    if (symbol != nullptr && symbol->Symbol() != nullptr && symbol->Symbol()[0] != L'\0') {
        return SafeText(symbol->Symbol());
    }
    return L"";
}

bool IsOpeningDeal(const IMTDeal* deal) {
    return deal != nullptr && deal->Entry() == IMTDeal::ENTRY_IN;
}

bool IsOrderTypeValid(const uint32_t type) {
    return type >= IMTConSymbol::MARGIN_RATE_FIRST && type <= IMTConSymbol::MARGIN_RATE_LAST;
}

uint32_t ResolveMarginRateType(
    const IMTExecution* execution,
    const IMTPosition* position,
    const IMTOrder* order,
    const IMTDeal* deal) {
    if (order != nullptr && IsOrderTypeValid(order->Type())) {
        return order->Type();
    }
    if (execution != nullptr && IsOrderTypeValid(execution->OrderType())) {
        return execution->OrderType();
    }
    if (deal != nullptr) {
        if (deal->Action() == IMTDeal::DEAL_BUY) {
            return IMTConSymbol::MARGIN_RATE_BUY;
        }
        if (deal->Action() == IMTDeal::DEAL_SELL) {
            return IMTConSymbol::MARGIN_RATE_SELL;
        }
    }
    if (position != nullptr) {
        if (position->Action() == IMTPosition::POSITION_BUY) {
            return IMTConSymbol::MARGIN_RATE_BUY;
        }
        if (position->Action() == IMTPosition::POSITION_SELL) {
            return IMTConSymbol::MARGIN_RATE_SELL;
        }
    }
    return UINT32_MAX;
}

const wchar_t* MarginRateTypeText(const uint32_t type) {
    switch (type) {
        case IMTConSymbol::MARGIN_RATE_BUY:
            return L"BUY";
        case IMTConSymbol::MARGIN_RATE_SELL:
            return L"SELL";
        case IMTConSymbol::MARGIN_RATE_BUY_LIMIT:
            return L"BUY_LIMIT";
        case IMTConSymbol::MARGIN_RATE_SELL_LIMIT:
            return L"SELL_LIMIT";
        case IMTConSymbol::MARGIN_RATE_BUY_STOP:
            return L"BUY_STOP";
        case IMTConSymbol::MARGIN_RATE_SELL_STOP:
            return L"SELL_STOP";
        case IMTConSymbol::MARGIN_RATE_BUY_STOP_LIMIT:
            return L"BUY_STOP_LIMIT";
        case IMTConSymbol::MARGIN_RATE_SELL_STOP_LIMIT:
            return L"SELL_STOP_LIMIT";
        default:
            return L"UNKNOWN";
    }
}

} // namespace

CPluginInstance::CPluginInstance() : api_(nullptr) {
}

CPluginInstance::~CPluginInstance() {
    Stop();
}

void CPluginInstance::Release() {
    delete this;
}

MTAPIRES CPluginInstance::Start(IMTServerAPI* server) {
    if (server == nullptr) {
        return MT_RET_ERR_PARAMS;
    }

    api_ = server;

    MTServerInfo info{};
    MTAPIRES rc = api_->About(info);
    if (rc != MT_RET_OK) {
        std::wstringstream audit;
        audit << L"Start About failed rc=" << rc;
        LogLine(api_, MTLogErr, audit.str());
    }

    rc = api_->TradeSubscribe(this);
    std::wstringstream audit;
    audit << L"Start TradeSubscribe rc=" << rc
          << L" target_login=" << kTargetLogin
          << L" plugin=" << kPluginName;
    LogLine(api_, rc == MT_RET_OK ? MTLogOK : MTLogErr, audit.str());
    return rc;
}

MTAPIRES CPluginInstance::Stop() {
    MTAPIRES rc = MT_RET_OK;
    IMTServerAPI* api = api_;
    if (api != nullptr) {
        rc = api->TradeUnsubscribe(this);
        std::wstringstream audit;
        audit << L"Stop TradeUnsubscribe rc=" << rc;
        LogLine(api, rc == MT_RET_OK ? MTLogOK : MTLogErr, audit.str());
    }
    api_ = nullptr;
    return rc;
}

MTAPIRES CPluginInstance::HookTradeRequestProcess(
    const IMTRequest* request,
    const IMTConfirm* /*confirm*/,
    const IMTConGroup* /*group*/,
    const IMTConSymbol* /*symbol*/,
    IMTPosition* position,
    IMTOrder* order,
    IMTDeal* deal) {
    const uint64_t login = ResolveLogin(nullptr, position, order, deal);
    const uint64_t request_login = request == nullptr ? 0 : request->Login();
    if (login != kTargetLogin && request_login != kTargetLogin) {
        return MT_RET_OK;
    }

    std::wstringstream audit;
    audit << L"HookTradeRequestProcess observed no-change"
          << L" request_login=" << request_login
          << L" resolved_login=" << login
          << L" action=" << (request == nullptr ? 0 : request->Action())
          << L" deal_entry=" << (deal == nullptr ? 999999 : deal->Entry());
    LogLine(api_, MTLogOK, audit.str());
    return MT_RET_OK;
}

MTAPIRES CPluginInstance::HookTradeExecution(
    const IMTConGateway* /*gateway*/,
    const IMTExecution* execution,
    const IMTConGroup* /*group*/,
    const IMTConSymbol* symbol,
    IMTPosition* position,
    IMTOrder* order,
    IMTDeal* deal) {
    const uint64_t login = ResolveLogin(execution, position, order, deal);
    if (login != kTargetLogin) {
        return MT_RET_OK;
    }

    const std::wstring symbol_name = ResolveSymbol(execution, position, order, deal, symbol);
    const std::tm tm = LocalServerTime();
    const int minute = tm.tm_min;
    const bool odd_minute = (minute % 2) == 1;
    const uint32_t margin_type = ResolveMarginRateType(execution, position, order, deal);

    std::wstringstream audit;
    audit << L"HookTradeExecution"
          << L" login=" << login
          << L" symbol=" << symbol_name
          << L" server_local_time=" << NowText(tm)
          << L" minute=" << minute
          << L" odd_minute=" << (odd_minute ? L"true" : L"false")
          << L" deal=" << (deal == nullptr ? 0 : deal->Deal())
          << L" deal_entry=" << (deal == nullptr ? 999999 : deal->Entry())
          << L" deal_action=" << (deal == nullptr ? 999999 : deal->Action())
          << L" order=" << (order == nullptr ? 0 : order->Order())
          << L" order_type=" << (order == nullptr ? 999999 : order->Type())
          << L" execution_order_type=" << (execution == nullptr ? 999999 : execution->OrderType())
          << L" position=" << (position == nullptr ? 0 : position->Position())
          << L" margin_type=" << margin_type
          << L" margin_type_name=" << MarginRateTypeText(margin_type);

    if (symbol == nullptr) {
        audit << L" action=skip reason=symbol-null";
        LogLine(api_, MTLogErr, audit.str());
        return MT_RET_OK;
    }

    if (!IsOrderTypeValid(margin_type)) {
        audit << L" action=skip reason=unknown-margin-type";
        LogLine(api_, MTLogErr, audit.str());
        return MT_RET_OK;
    }

#if POC_REQUIRE_OPENING_DEAL
    if (!IsOpeningDeal(deal)) {
        audit << L" action=skip reason=not-opening-deal";
        LogLine(api_, MTLogOK, audit.str());
        return MT_RET_OK;
    }
#endif

    const double before = symbol->MarginRateMaintenance(margin_type);
    audit << L" maintenance_before=" << before;

    if (!odd_minute) {
        audit << L" action=no-change";
        LogLine(api_, MTLogOK, audit.str());
        return MT_RET_OK;
    }

    if (!std::isfinite(before)) {
        audit << L" action=skip reason=non-finite-maintenance-rate";
        LogLine(api_, MTLogErr, audit.str());
        return MT_RET_OK;
    }

    const double base = before > 0.0 ? before : kUnsetRateBase;
    const double after = base * kOddMinuteFactor;

    IMTConSymbol* mutable_symbol = const_cast<IMTConSymbol*>(symbol);
    const MTAPIRES rc = mutable_symbol->MarginRateMaintenance(margin_type, after);
    const double observed_after = symbol->MarginRateMaintenance(margin_type);

    audit << L" base=" << base
          << L" factor=" << kOddMinuteFactor
          << L" maintenance_after_target=" << after
          << L" setter_rc=" << rc
          << L" maintenance_after_observed=" << observed_after
          << L" action=const-cast-symbol-maintenance-rate";
    LogLine(api_, rc == MT_RET_OK ? MTLogOK : MTLogErr, audit.str());
    return rc == MT_RET_OK ? MT_RET_OK : rc;
}

void CPluginInstance::OnTradeExecution(
    const IMTConGateway* /*gateway*/,
    const IMTExecution* execution,
    const IMTConGroup* /*group*/,
    const IMTConSymbol* symbol,
    const IMTPosition* position,
    const IMTOrder* order,
    const IMTDeal* deal) {
    const uint64_t login = ResolveLogin(execution, position, order, deal);
    if (login != kTargetLogin) {
        return;
    }

    const uint32_t margin_type = ResolveMarginRateType(execution, position, order, deal);
    std::wstringstream audit;
    audit << L"OnTradeExecution observed"
          << L" login=" << login
          << L" symbol=" << ResolveSymbol(execution, position, order, deal, symbol)
          << L" deal=" << (deal == nullptr ? 0 : deal->Deal())
          << L" deal_entry=" << (deal == nullptr ? 999999 : deal->Entry())
          << L" order=" << (order == nullptr ? 0 : order->Order())
          << L" position=" << (position == nullptr ? 0 : position->Position())
          << L" margin_type=" << margin_type
          << L" margin_type_name=" << MarginRateTypeText(margin_type);

    if (symbol != nullptr && IsOrderTypeValid(margin_type)) {
        audit << L" maintenance_rate=" << symbol->MarginRateMaintenance(margin_type);
    }
    LogLine(api_, MTLogOK, audit.str());
}
