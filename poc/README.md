# MinuteParityMargin PoC

这个 PoC 只验证一个问题：在 MT5 Server Plugin 的交易执行 hook 中，修改 `IMTOrder/IMTDeal/IMTPosition::RateMargin` 后，是否会影响 MT5 账户最终可见保证金。

## 行为

- 只对 `login == 15031` 生效。
- 只处理新开仓成交，即 `IMTDeal::Entry() == ENTRY_IN`。
- 使用插件所在服务器进程的本地时间取当前分钟数。
- 当前分钟数为奇数时，把本次开仓相关的 `order/deal/position` 的 `RateMargin` 乘以 `2`。
- 当前分钟数为偶数时，完全不改任何对象。
- 不处理大腿方向、持久化、挂单取消、平仓、部分平仓、close-by。

MetaQuotes 文档里 `RateMargin` 的定义是“交易品种/交易/持仓预付款货币到客户存款货币的汇率”，不是业务意义上的杠杆或 margin multiplier。本 PoC 是有意修改这个汇率字段，验证它是否能作为“让最终保证金翻倍”的可用技术入口。

## 构建

带真实 MT5 Server API SDK 构建：

```powershell
cmake -S . -B build `
  -DMT5_SERVER_API_INCLUDE_DIR="C:/path/to/mt5/server/api/include" `
  -DMT5_SERVER_API_LIBRARY="C:/path/to/mt5/server/api/lib/MT5APIServer64.lib"
cmake --build build --config Release
```

只做本地语法检查可以打开 stub：

```bash
cmake -S . -B build-smoke -DPOC_USE_STUB_MT5_API=ON
cmake --build build-smoke
```

stub 产物不能安装到 MT5，只用于检查 PoC 代码结构。

如果默认版验证能让保证金翻倍，可以进一步分别验证哪个对象字段真正生效：

```powershell
cmake -S . -B build-order-only `
  -DMT5_SERVER_API_INCLUDE_DIR="C:/path/to/mt5/server/api/include" `
  -DMT5_SERVER_API_LIBRARY="C:/path/to/mt5/server/api/lib/MT5APIServer64.lib" `
  -DPOC_SET_ORDER_RATE_MARGIN=ON `
  -DPOC_SET_DEAL_RATE_MARGIN=OFF `
  -DPOC_SET_POSITION_RATE_MARGIN=OFF
cmake --build build-order-only --config Release
```

`POC_SET_ORDER_RATE_MARGIN`、`POC_SET_DEAL_RATE_MARGIN`、`POC_SET_POSITION_RATE_MARGIN` 三个开关默认都为 `ON`。

## 日志

插件会在运行目录追加写入：

```text
minute_parity_margin_poc.log
```

日志会记录 hook 名称、login、symbol、服务器本地时间、分钟数、是否奇数、deal/order/position 编号，以及修改前后的 `RateMargin`。
