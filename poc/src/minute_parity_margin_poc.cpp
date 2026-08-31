#include "mt5_api_poc_compat.h"

#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
#define POC_EXPORT extern "C" __declspec(dllexport)
#define POC_APIENTRY __stdcall
#else
#define POC_EXPORT extern "C" __attribute__((visibility("default")))
#define POC_APIENTRY
#endif

#ifndef POC_SET_ORDER_RATE_MARGIN
#define POC_SET_ORDER_RATE_MARGIN 1
#endif

#ifndef POC_SET_DEAL_RATE_MARGIN
#define POC_SET_DEAL_RATE_MARGIN 1
#endif

#ifndef POC_SET_POSITION_RATE_MARGIN
#define POC_SET_POSITION_RATE_MARGIN 1
#endif

namespace {

constexpr UINT64 kTargetLogin = 15031;
constexpr double kOddMinuteRateFactor = 2.0;
constexpr const wchar_t* kPluginName = L"MinuteParityMargin.PoC";

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

void LogLine(const std::wstring& line) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    const std::tm tm = LocalServerTime();
    std::wofstream file("minute_parity_margin_poc.log", std::ios::app);
    file << NowText(tm) << L" " << line << L"\n";

#ifdef _WIN32
    const std::wstring debug = line + L"\n";
    OutputDebugStringW(debug.c_str());
#endif
}

bool IsOpenDeal(const IMTDeal* deal) {
    if (deal == nullptr) {
        return false;
    }
    return deal->Entry() == IMTDeal::ENTRY_IN;
}

UINT64 ResolveLogin(
    const IMTRequest* request,
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
    if (request != nullptr && request->Login() != 0) {
        return request->Login();
    }
    if (execution != nullptr && execution->Login() != 0) {
        return execution->Login();
    }
    return 0;
}

std::wstring ResolveSymbol(
    const IMTRequest* request,
    const IMTExecution* execution,
    const IMTPosition* position,
    const IMTOrder* order,
    const IMTDeal* deal) {
    if (deal != nullptr && deal->Symbol() != nullptr && deal->Symbol()[0] != L'\0') {
        return SafeText(deal->Symbol());
    }
    if (position != nullptr && position->Symbol() != nullptr && position->Symbol()[0] != L'\0') {
        return SafeText(position->Symbol());
    }
    if (order != nullptr && order->Symbol() != nullptr && order->Symbol()[0] != L'\0') {
        return SafeText(order->Symbol());
    }
    if (request != nullptr && request->Symbol() != nullptr && request->Symbol()[0] != L'\0') {
        return SafeText(request->Symbol());
    }
    if (execution != nullptr && execution->Symbol() != nullptr && execution->Symbol()[0] != L'\0') {
        return SafeText(execution->Symbol());
    }
    return L"";
}

template <typename TObject>
MTAPIRES MultiplyRateMargin(
    const wchar_t* object_name,
    TObject* object,
    double factor,
    std::wstringstream& audit) {
    if (object == nullptr) {
        audit << L" " << object_name << L"=null";
        return MT_RET_OK;
    }

    const double before = object->RateMargin();
    if (!std::isfinite(before) || before <= 0.0) {
        audit << L" " << object_name << L".rate_margin_skip=" << before;
        return MT_RET_OK;
    }

    const double after = before * factor;
    const MTAPIRES rc = object->RateMargin(after);
    audit << L" " << object_name << L".rate_margin=" << before << L"->" << after
          << L" rc=" << rc;
    return rc;
}

class MinuteParityMarginPoc final : public IMTServerPlugin, public IMTTradeSink {
public:
    void Release() override {
        delete this;
    }

    MTAPIRES Start(IMTServerAPI* server) override {
        if (server == nullptr) {
            return MT_RET_ERR_PARAMS;
        }
        api_ = server;
        const MTAPIRES rc = api_->TradeSubscribe(this);
        std::wstringstream audit;
        audit << L"Start TradeSubscribe rc=" << rc << L" target_login=" << kTargetLogin;
        LogLine(audit.str());
        return rc;
    }

    MTAPIRES Stop() override {
        MTAPIRES rc = MT_RET_OK;
        if (api_ != nullptr) {
            rc = api_->TradeUnsubscribe(this);
        }
        api_ = nullptr;
        std::wstringstream audit;
        audit << L"Stop TradeUnsubscribe rc=" << rc;
        LogLine(audit.str());
        return rc;
    }

    MTAPIRES HookTradeRequestProcess(
        const IMTRequest* request,
        const IMTConfirm* /*confirm*/,
        const IMTConGroup* /*group*/,
        const IMTConSymbol* /*symbol*/,
        IMTPosition* position,
        IMTOrder* order,
        IMTDeal* deal) override {
        return ApplyMinuteRule(
            L"HookTradeRequestProcess",
            request,
            nullptr,
            position,
            order,
            deal);
    }

    MTAPIRES HookTradeRequestProcessCloseBy(
        const IMTRequest* request,
        const IMTConfirm* /*confirm*/,
        const IMTConGroup* /*group*/,
        const IMTConSymbol* /*symbol*/,
        IMTPosition* position,
        IMTOrder* order,
        IMTDeal* deal,
        IMTDeal* deal_by) override {
        std::wstringstream audit;
        audit << L"HookTradeRequestProcessCloseBy no-op"
              << L" login=" << ResolveLogin(request, nullptr, position, order, deal)
              << L" position_by=" << (request == nullptr ? 0 : request->PositionBy())
              << L" deal_by=" << (deal_by == nullptr ? 0 : deal_by->Deal());
        LogLine(audit.str());
        return MT_RET_OK;
    }

    MTAPIRES HookTradeExecution(
        const IMTConGateway* /*gateway*/,
        const IMTExecution* execution,
        const IMTConGroup* /*group*/,
        const IMTConSymbol* /*symbol*/,
        IMTPosition* position,
        IMTOrder* order,
        IMTDeal* deal) override {
        return ApplyMinuteRule(
            L"HookTradeExecution",
            nullptr,
            execution,
            position,
            order,
            deal);
    }

private:
    MTAPIRES ApplyMinuteRule(
        const wchar_t* hook_name,
        const IMTRequest* request,
        const IMTExecution* execution,
        IMTPosition* position,
        IMTOrder* order,
        IMTDeal* deal) {
        const UINT64 login = ResolveLogin(request, execution, position, order, deal);
        const std::wstring symbol = ResolveSymbol(request, execution, position, order, deal);

        if (login != kTargetLogin) {
            return MT_RET_OK;
        }

        if (!IsOpenDeal(deal)) {
            std::wstringstream skipped;
            skipped << hook_name << L" skip non-opening"
                    << L" login=" << login
                    << L" symbol=" << symbol
                    << L" deal=" << (deal == nullptr ? 0 : deal->Deal())
                    << L" entry=" << (deal == nullptr ? 999999 : deal->Entry());
            LogLine(skipped.str());
            return MT_RET_OK;
        }

        const std::tm tm = LocalServerTime();
        const int minute = tm.tm_min;
        const bool odd_minute = (minute % 2) == 1;
        const double factor = odd_minute ? kOddMinuteRateFactor : 1.0;

        std::wstringstream audit;
        audit << hook_name
              << L" login=" << login
              << L" symbol=" << symbol
              << L" server_local_time=" << NowText(tm)
              << L" minute=" << minute
              << L" odd_minute=" << (odd_minute ? L"true" : L"false")
              << L" factor=" << factor
              << L" deal=" << (deal == nullptr ? 0 : deal->Deal())
              << L" order=" << (order == nullptr ? 0 : order->Order())
              << L" position=" << (position == nullptr ? 0 : position->Position());

        if (!odd_minute) {
            audit << L" action=no-change";
            LogLine(audit.str());
            return MT_RET_OK;
        }

        MTAPIRES rc = MT_RET_OK;
#if POC_SET_ORDER_RATE_MARGIN
        const MTAPIRES order_rc = MultiplyRateMargin(L"order", order, factor, audit);
        if (order_rc != MT_RET_OK) {
            rc = order_rc;
        }
#else
        audit << L" order.rate_margin=disabled";
#endif

#if POC_SET_DEAL_RATE_MARGIN
        const MTAPIRES deal_rc = MultiplyRateMargin(L"deal", deal, factor, audit);
        if (rc == MT_RET_OK && deal_rc != MT_RET_OK) {
            rc = deal_rc;
        }
#else
        audit << L" deal.rate_margin=disabled";
#endif

#if POC_SET_POSITION_RATE_MARGIN
        const MTAPIRES position_rc = MultiplyRateMargin(L"position", position, factor, audit);
        if (rc == MT_RET_OK && position_rc != MT_RET_OK) {
            rc = position_rc;
        }
#else
        audit << L" position.rate_margin=disabled";
#endif

        audit << L" final_rc=" << rc;
        LogLine(audit.str());
        return rc;
    }

    IMTServerAPI* api_ = nullptr;
};

MTPluginInfo kPluginInfo = {
    100,
    0,
    kPluginName,
    L"Internal PoC",
    L"PoC: double margin-currency conversion rate on odd minutes for login 15031 opening deals"
};

} // namespace

POC_EXPORT MTAPIRES POC_APIENTRY MTServerAbout(MTPluginInfo& info) {
    info = kPluginInfo;
    return MT_RET_OK;
}

POC_EXPORT MTAPIRES POC_APIENTRY MTServerCreate(UINT /*apiversion*/, IMTServerPlugin** plugin) {
    if (plugin == nullptr) {
        return MT_RET_ERR_PARAMS;
    }

    *plugin = new MinuteParityMarginPoc();
    return *plugin == nullptr ? MT_RET_ERROR : MT_RET_OK;
}

POC_EXPORT MTAPIRES POC_APIENTRY MTServerInit(UINT /*apiversion*/) {
    return MT_RET_OK;
}
