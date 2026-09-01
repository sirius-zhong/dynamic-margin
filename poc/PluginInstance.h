#pragma once

#include "stdafx.h"

class CPluginInstance final : public IMTServerPlugin, public IMTTradeSink {
public:
    CPluginInstance();
    ~CPluginInstance();

    void Release() override;
    MTAPIRES Start(IMTServerAPI* server) override;
    MTAPIRES Stop() override;

    MTAPIRES HookTradeRequestProcess(
        const IMTRequest* request,
        const IMTConfirm* confirm,
        const IMTConGroup* group,
        const IMTConSymbol* symbol,
        IMTPosition* position,
        IMTOrder* order,
        IMTDeal* deal) override;

    MTAPIRES HookTradeExecution(
        const IMTConGateway* gateway,
        const IMTExecution* execution,
        const IMTConGroup* group,
        const IMTConSymbol* symbol,
        IMTPosition* position,
        IMTOrder* order,
        IMTDeal* deal) override;

    void OnTradeExecution(
        const IMTConGateway* gateway,
        const IMTExecution* execution,
        const IMTConGroup* group,
        const IMTConSymbol* symbol,
        const IMTPosition* position,
        const IMTOrder* order,
        const IMTDeal* deal) override;

private:
    IMTServerAPI* api_;
};
