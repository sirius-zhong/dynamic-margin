# PoC 验证清单

## P0：`HookTradeExecution` 是否会被目标交易触发

1. 确认插件启动成功，`symbol_maintenance_margin_poc.log` 出现 `Start TradeSubscribe rc=0 target_login=15031`。
2. 用 `15031` 通过目标路由开 0.01 手 XAUUSD。
3. 如果只出现 `HookTradeRequestProcess observed no-change`，没有 `HookTradeExecution`，说明该交易路径没有进入 gateway execution hook，本 PoC 对这条路径不会生效。
4. 如果出现 `HookTradeExecution`，继续看 `deal_entry`、`margin_type`、`maintenance_before`、`setter_rc`。

## P0：修改 `symbol->MarginRateMaintenance(type)` 是否影响最终保证金

1. 在偶数分钟用 `15031` 开 0.01 手 XAUUSD，记录开仓前后 MT5 Manager/Terminal/API 里的账户 `Margin`、`FreeMargin`、`MarginLevel`。
2. 在奇数分钟用 `15031` 开同样 0.01 手 XAUUSD，确认日志里 `action=const-cast-symbol-maintenance-rate` 且 `setter_rc=0`。
3. 对比偶数分钟和奇数分钟的新增保证金。如果正常为 8，奇数分钟预期应接近 16。
4. 如果 `setter_rc=0` 且 `maintenance_after_observed` 已变成目标值，但账户保证金仍约为 8，则说明 hook 内修改 symbol 不是本次账户保证金计算的有效输入。
5. 如果账户保证金变成约 16，继续验证该值是否会在价格刷新、重新登录、服务器重启后保持一致。

## P0：是否污染 symbol 全局配置

1. 奇数分钟触发一次 `15031` 的 XAUUSD 开仓。
2. 立即用其他账户开同 symbol 同方向小仓位，确认是否也使用被修改后的 `MarginRateMaintenance`。
3. 在 MT5 管理端查看 symbol 配置是否被永久改写。
4. 如果其他账户或后续交易也被影响，说明该入口即使能改变保证金，也不满足“只对单个账户/单个新开仓 position 生效”的需求。

## P0：是否影响 stop-out 判断

1. 准备一个保证金率接近 stop-out 阈值的测试账户。
2. 在奇数分钟开仓，让 PoC 触发 `MarginRateMaintenance * 2`。
3. 确认 MT5 stop-out 计算使用的是翻倍后的 `Margin`，而不是原生保证金。
4. 如果账户展示翻倍但 stop-out 仍按原生值判断，本方案仍不能满足完整需求。

## P1：时间来源是否等于 MT5 交易服务器时间

1. 对比日志里的 `server_local_time` 与 MT5 Manager/Terminal 显示的交易服务器时间。
2. 如果分钟数不一致，需要把 `LocalServerTime()` 替换为 MT5 SDK 提供的服务器时间 API，再重新测试。

## P1：deal 过滤是否过严

1. 默认构建要求 `deal != nullptr && deal->Entry() == ENTRY_IN` 才会修改 symbol。
2. 如果 `HookTradeExecution` 日志显示目标开仓里 `deal=null` 或 `deal_entry` 不是 `ENTRY_IN`，可以在隔离环境关闭 `POC_REQUIRE_OPENING_DEAL` 再测一次。
3. 关闭过滤后必须只在测试账户、测试 symbol、测试时段验证，避免平仓或其他 execution 误触发配置修改。

## P1：Hook 覆盖范围

1. 市价开仓是否进入 `HookTradeExecution`。
2. 挂单触发开仓是否进入 `HookTradeExecution`，并能触发同样的日志。
3. 外部 gateway execution 是否进入 `HookTradeExecution`。
4. 其他账号开仓不应出现 `action=const-cast-symbol-maintenance-rate`。
5. `15031` 平仓、部分平仓、close-by 默认不应触发 symbol 修改，只允许出现 skip 日志。

## P1：维持保证金率 vs 初始保证金率

1. 本 PoC 只改 `MarginRateMaintenance`，不改 `MarginRateInitial`。
2. 如果新开仓保证金只看初始保证金率，那么奇数分钟开仓保证金可能不会变化。
3. 如果维持保证金率只影响持仓后续重算或 stop-out 阶段，则需要继续做第二个 PoC，同时修改 `MarginRateInitial` 和 `MarginRateMaintenance` 对比。

## P2：0 值语义

1. 如果日志里 `maintenance_before=0`，PoC 会按 `base=1` 设置目标值为 `2`。
2. 需要确认 MT5 中 `0` 表示“未配置/默认值”，还是实际的 0 倍率。
3. 如果 0 有特殊语义，生产方案不能简单把 0 当 1。
