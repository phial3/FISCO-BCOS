### 3.0.0-rc3
(2022-03-31)

**新增**

- 支持Solidity合约并行冲突字段分析
- 将密码学、交易编解码等相关逻辑整合到bcos-cpp-sdk中，并封装成通用的C接口
- WASM虚拟机支持合约调用合约
- 新增bcos-wasm替代Hera
- `BFS`支持软链接功能
- 支持通过`setSystemConfig`系统合约的`tx_gas_limit`关键字动态修改交易执行的gas限制
- 部署合约存储合约ABI


**更改**

- 升级EVM虚拟机到最新，支持Solidity 0.8
- 机构层面优化网络广播，减少机构间外网带宽占用
- 支持国密加速库，国密签名和验签性能提升5-10倍
- EVM节点支持`BFS`，使用`BFS`替代`CNS`
- DAG框架统一支持Solidity和WBC-Liquid

**修复**

- 修复[#issue 2312](https://github.com/FISCO-BCOS/FISCO-BCOS/issues/2312)
- 修复[#issue 2307](https://github.com/FISCO-BCOS/FISCO-BCOS/issues/2307)
- 修复[#issue 2254](https://github.com/FISCO-BCOS/FISCO-BCOS/issues/2254)
- 修复[#issue 2211](https://github.com/FISCO-BCOS/FISCO-BCOS/issues/2211)
- 修复[#issue 2195](https://github.com/FISCO-BCOS/FISCO-BCOS/issues/2195)

**兼容性**

3.0.0-rc3版本与3.0.0-rc2版本数据和协议不兼容，Solidity/WBC-Liquid合约源码兼容。如果要从3.0.0-rc2版本升级到3.0.0-rc3版本，需要做数据迁移。

|            | 推荐版本                | 最低版本  | 说明                   |
| ---------- | ----------------------- | --------- | ---------------------- |
| 控制台     | 3.0.0-rc3                  | 3.0.0-rc2 (CNS功能无法使用)    |                        |
| Java SDK        | 3.0.0-rc3           | 3.0.0-rc2 (CNS功能无法使用)     |     |
| CPP SDK        | 3.0.0-rc3           | 3.0.0-rc2     |     |
| WeBASE     | 暂时不支持(预计lab-rc3版本支持)         | 暂时不支持(预计lab-rc2版本支持) | |
| Solidity   | 最高支持 solidity 0.8.11.0 | 0.6.10    |                        |
| Liquid     | 1.0.0-rc3               | 1.0.0-rc2  |                      |


### 3.0.0-rc2
(2022-02-23)

**更改**

- 优化代码仓库管理复杂度，将多个子仓库集中到FISCO BCOS统一管理
- 交易由`Base64`编码修改为十六进制编码
- 升级`bcos-boostssl`和`bcos-utilities`依赖到最新版本
- 修改`bytesN`类型数据的Scale编解码
- 优化交易处理流程，避免交易多次重复验签导致的性能损耗
- 优化事件推送模块的块高获取方法


**修复**

- 修复scheduler调度交易过程中导致的内存泄露
- 修复DMC+DAG调度过程中执行不一致的问题
- 修复[Issue 2132](https://github.com/FISCO-BCOS/FISCO-BCOS/issues/2132)
- 修复[Issue 2124](https://github.com/FISCO-BCOS/FISCO-BCOS/issues/2124)
- 修复部分场景新节点入网没有触发快速视图切换，导致节点数满足`(2*f+1)`却共识异常的问题
- 修复部分变量访问线程不安全导致节点崩溃的问题
- 修复AMOP订阅多个topics失败的问题

**兼容性**

3.0.0-rc2版本与3.0.0-rc1版本数据和协议不兼容，Solidity/WBC-Liquid合约源码兼容。如果要从3.0.0-rc1版本升级到3.0.0-rc2版本，需要做数据迁移。

|            | 推荐版本                | 最低版本  | 说明                   |
| ---------- | ----------------------- | --------- | ---------------------- |
| 控制台     | 3.0.0-rc2                  | 3.0.0-rc2     |                        |
| Java SDK        | 3.0.0-rc2           | 3.0.0-rc2     |     |
| CPP SDK        | 3.0.0-rc2           | 3.0.0-rc2     |     |
| WeBASE     | 暂时不支持(预计lab-rc2版本支持)         | 暂时不支持(预计lab-rc2版本支持) | |
| Solidity   | 最高支持 solidity 0.6.10 | 0.6.10    |                        |
| Liquid     | 1.0.0-rc2               | 1.0.0-rc2  |                      |

### 3.0.0-rc1
(2021-12-09)

**新增**

## 微服务架构
- 提供通用的区块链接入规范。
- 提供管理平台，用户可以一键部署、扩容、获得接口粒度的监控信息。

## 确定性多合约并行
- 易用：区块链底层自动并行，无需使用者预先提供冲突字段。
- 高效：区块内的交易不重复执行，没有预执行或预分析的流程。
- 通用：无论 EVM、WASM、Precompiled 或其它合约，都能使用此方案。

## 区块链文件系统
- 引入文件系统概念来组织链上资源，用户可以像浏览文件一样浏览链上资源。
- 基于区块链文件系统实现管理功能，如分区、权限等，更直观。

## 流水线PBFT共识
- 交易排序与交易执行相互独立，实现流水线架构，提升资源利用率。
- 支持批量共识，对区间内区块并行共识处理，提升性能。
- 支持单个共识Leader连续出块，提升性能。

## WBC-Liquid
- 集成WASM运行环境，支持Liquid智能合约。
- Liquid智能合约支持智能分析冲突字段，自动开启DAG。

**修复**

**兼容性**

3.0版本与2.0版本数据和协议不兼容，Solidity合约源码兼容。如果要从2.0版本升级到3.0版本，需要做数据迁移。

|            | 推荐版本                | 最低版本  | 说明                   |
| ---------- | ----------------------- | --------- | ---------------------- |
| 控制台     | 3.0.0-rc1                  | 3.0.0-rc1     |                        |
| Java SDK        | 3.0.0-rc1           | 3.0.0-rc1     |     |
| CPP SDK        | 3.0.0-rc1           | 3.0.0-rc1     |     |
| WeBASE     | lab-rc1                   | lab-rc1 |                        |
| Solidity   | 最高支持 solidity 0.6.10 | 0.6.10    |                        |
| Liquid     | 1.0.0-rc2               | 1.0.0-rc2  |                      |