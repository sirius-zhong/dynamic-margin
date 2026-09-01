# SymbolMaintenanceMargin PoC

这个 PoC 只验证一个问题：在 MT5 Server Plugin 的 `HookTradeExecution` 中，尝试修改当前 hook 传入的 `IMTConSymbol::MarginRateMaintenance(type)` 后，是否会影响 MT5 对本次 execution 的账户保证金计算。

注意：真实 `MT5APIServer.h` 中 `HookTradeExecution` 的 `symbol` 参数类型是 `const IMTConSymbol*`，但 `MarginRateMaintenance(type, value)` 是非 const setter。因此本 PoC 会有意使用 `const_cast`。这只是为了验证 MT5 Server 内部是否允许通过该 hook 里的 symbol 快照影响计算，不代表生产方案可直接采用。

## 行为

- 只对 `login == 15031` 生效。
- 默认只处理新开仓成交，即 `IMTDeal::Entry() == ENTRY_IN`。
- 使用插件所在服务器进程的本地时间取当前分钟数。
- 当前分钟数为奇数时，读取本次交易方向对应的 `symbol->MarginRateMaintenance(type)`，并尝试设置为 `base * 2`。
- 如果原始 `MarginRateMaintenance(type) <= 0`，PoC 会把它视为未显式配置，并按 `base = 1.0` 处理，因此会尝试设置成 `2.0`。
- 当前分钟数为偶数时，完全不改任何对象。
- 不处理大腿方向、持久化、挂单取消、平仓、部分平仓、close-by。
- `HookTradeRequestProcess` 只打 no-op 日志，用于确认交易是否根本没有走到 `HookTradeExecution`。

`MarginRateMaintenance(type)` 是按订单类型分槽位的字段，type 与 `IMTOrder::EnOrderType` / `IMTConSymbol::EnMarginRateTypes` 对齐，例如 `0=BUY`、`1=SELL`。

## 重要风险

- 如果 `symbol` 是共享配置对象而不是本次 execution 的临时快照，奇数分钟设置的维持保证金率可能会污染该 symbol 后续所有账户的计算。
- 如果 setter 返回成功但本次账户 `Margin / FreeMargin / MarginLevel` 不变，说明这个 hook 里的 symbol 修改不是保证金计算输入。
- 如果只在 `OnTradeExecution` 日志里看到修改值，但下一次价格刷新或重算后恢复/失效，说明它最多影响短暂对象状态，不能满足最终需求。
- 这个 PoC 必须只在隔离测试环境和隔离 symbol 上验证。

## 代码结构

PoC 代码按 MT5 官方 Server example 的习惯平铺在当前目录：

- `SymbolMaintenanceMargin.cpp`：DLL 入口、`MTServerAbout`、`MTServerCreate`
- `PluginInstance.h`：插件实例类声明
- `PluginInstance.cpp`：`Start/Stop`、`HookTradeExecution`、`OnTradeExecution` 逻辑
- `pch.h/.cpp`、`framework.h`：兼容 Visual Studio 默认 DLL 项目的预编译头入口，内部会包含 `stdafx.h`
- `stdafx.h/.cpp`：对齐 MT5 官方 Server example 的预编译头/SDK 包含习惯
- `mt5_api_poc_compat.h`：本地 stub / 真实 `MT5APIServer.h` 兼容入口

Visual Studio 工程 `poc.vcxproj` 里已经移除了默认模板生成的 `dllmain.cpp` 编译项，因为插件的 `DllMain` 已在 `SymbolMaintenanceMargin.cpp` 中实现。参与编译的 `.cpp` 第一行需要保持为 `#include "pch.h"`，否则开启预编译头时会触发 `C1010`。

## 构建

带真实 MT5 Server API SDK 构建：

```powershell
cmake -S . -B build `
  -DMT5_SERVER_API_INCLUDE_DIR="C:/path/to/MetaTrader5SDK/Include" `
  -DMT5_SERVER_API_LIBRARY="C:/path/to/MetaTrader5SDK/Libs/MT5APIServer64.lib"
cmake --build build --config Release
```

只做本地语法检查可以打开 stub：

```bash
cmake -S . -B build-smoke -DPOC_USE_STUB_MT5_API=ON
cmake --build build-smoke
```

stub 产物不能安装到 MT5，只用于检查 PoC 代码结构。

如果 `HookTradeExecution` 中 `deal` 为空或 `deal->Entry()` 不是 `ENTRY_IN`，但你仍想在隔离环境里验证 symbol 修改是否能生效，可以关闭开仓成交过滤：

```powershell
cmake -S . -B build-no-entry-filter `
  -DMT5_SERVER_API_INCLUDE_DIR="C:/path/to/MetaTrader5SDK/Include" `
  -DMT5_SERVER_API_LIBRARY="C:/path/to/MetaTrader5SDK/Libs/MT5APIServer64.lib" `
  -DPOC_REQUIRE_OPENING_DEAL=OFF
cmake --build build-no-entry-filter --config Release
```

`POC_REQUIRE_OPENING_DEAL` 默认 `ON`。

## 日志

插件会在运行目录追加写入：

```text
symbol_maintenance_margin_poc.log
```

日志会同时写入 MT5 Server 日志和本地文件，记录 hook 名称、login、symbol、服务器本地时间、分钟数、deal/order/position 编号、推导出的 margin rate type、修改前后的 `MarginRateMaintenance` 和 setter 返回码。
