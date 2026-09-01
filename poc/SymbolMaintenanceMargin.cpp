#include "stdafx.h"
#include "PluginInstance.h"

#include <new>

namespace {

MTPluginInfo ExtPluginInfo = {
    100,
    MTServerAPIVersion,
    L"SymbolMaintenanceMargin.PoC",
    L"Internal PoC",
    L"PoC: modify IMTConSymbol::MarginRateMaintenance in HookTradeExecution for login 15031"
};

} // namespace

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE /*module*/, DWORD reason, LPVOID /*reserved*/) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif

MTAPIENTRY MTAPIRES MTServerAbout(MTPluginInfo& info) {
    info = ExtPluginInfo;
    return MT_RET_OK;
}

MTAPIENTRY MTAPIRES MTServerCreate(uint32_t /*apiversion*/, IMTServerPlugin** plugin) {
    if (plugin == nullptr) {
        return MT_RET_ERR_PARAMS;
    }

    *plugin = new (std::nothrow) CPluginInstance();
    return *plugin == nullptr ? MT_RET_ERR_MEM : MT_RET_OK;
}
