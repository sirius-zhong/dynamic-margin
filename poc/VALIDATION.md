# PoC 验证清单

## P0：`RateMargin` 是否影响最终保证金

1. 确认插件启动成功，`minute_parity_margin_poc.log` 出现 `Start TradeSubscribe rc=0 target_login=15031`。
2. 在偶数分钟用 `15031` 开 0.01 手 XAUUSD，记录开仓前后 MT5 Manager/Terminal/API 里账户 `Margin`、`FreeMargin`、`MarginLevel`。
3. 在奇数分钟用 `15031` 开同样 0.01 手 XAUUSD，确认日志里 `factor=2`，并记录同样字段。
4. 对比偶数分钟和奇数分钟的新增保证金。如果正常为 8，奇数分钟预期应接近 16。
5. 若日志显示 `RateMargin` 已从 `1 -> 2`，但账户保证金仍约为 8，则说明 hook 中改 `RateMargin` 不能作为最终保证金入口。
6. 若账户保证金变成约 16，继续验证该值是否会在价格刷新、重新登录、服务器重启后保持一致。

## P0：是否影响 stop-out 判断

1. 准备一个保证金率接近 stop-out 阈值的测试账户。
2. 在奇数分钟开仓，让 PoC 触发 `RateMargin * 2`。
3. 确认 MT5 stop-out 计算使用的是翻倍后的 `Margin`，而不是原生保证金。
4. 如果账户展示翻倍但 stop-out 仍按原生值判断，本方案仍不能满足完整需求。

## P1：时间来源是否等于 MT5 交易服务器时间

1. 对比日志里的 `server_local_time` 与 MT5 Manager/Terminal 显示的交易服务器时间。
2. 如果分钟数不一致，需要把 `LocalServerTime()` 替换为 MT5 SDK 提供的服务器时间 API，再重新测试。

## P1：Hook 覆盖范围

1. 市价开仓是否进入 `HookTradeRequestProcess`。
2. 挂单触发开仓是否进入 `HookTradeRequestProcess` 或 `HookTradeExecution`，并能触发同样的日志。
3. 外部 gateway execution 是否进入 `HookTradeExecution`。
4. 其他账号开仓不应出现 `RateMargin` 修改。
5. `15031` 平仓、部分平仓、close-by 不应触发 `RateMargin` 修改，只允许出现 no-op 日志。

## P1：字段语义风险

1. 验证 XAUUSD 的 margin currency 与账户 deposit currency 相同或不同两种情况。
2. 如果不同币种下 `RateMargin * 2` 产生汇率语义污染，说明生产方案不能直接使用这个字段表达业务高风险保证金率。
3. 如果该字段只影响历史记录或报表字段，不影响实时账户 margin，则必须放弃该入口。

## P1：定位具体生效对象

默认 PoC 会同时修改 `order/deal/position` 三个对象的 `RateMargin`。如果默认版能让保证金翻倍，需要再分别构建以下三个版本，确认最小生效字段：

1. 只打开 `POC_SET_ORDER_RATE_MARGIN`。
2. 只打开 `POC_SET_DEAL_RATE_MARGIN`。
3. 只打开 `POC_SET_POSITION_RATE_MARGIN`。

如果只有 `order` 生效，说明必须在执行前订单阶段介入；如果只有 `position` 生效，说明后续 `PositionUpdate` 可能值得继续验证；如果三者都不生效，则这条技术路径不可用。
