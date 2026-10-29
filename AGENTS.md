# qPCRtools_cpp 项目规则

## 项目根路径

- 本地根目录：`/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp`
- 超算端：无

## 项目目标

- C++17/Qt6 桌面版 qPCR 分析软件。
- 当前主线是 Bioinformatics Application Note 的返修稿、独立数值验证和可复现软件发布。
- 软件核心在 `code/`，稿件在 `manuscript/`，历史材料在 `archive/`。

## 关键约定

- 执行全局 `AGENTS.md` 的代码、论文、绘图、引用核验、文件安全和记录规则。
- 软件版本以 `code/CMakeLists.txt` 的 `project(... VERSION ...)` 为唯一来源。
- 数值验证以独立 base-R 参照为准，不以旧版 R 包的数值一致作为正确性证据。
- 统计检验尺度、多数比较校正方法和原始/校正 P 值必须在结果表与分析记录中可追溯。
- 不删除用户文件；归档只使用保留原文件的移动操作，并同步更新 `archive/00_README.md`。
- 主文、补充材料、Word、图注、Cover letter 和验证结果必须在同一版本上交叉核对。
- 本项目的本地路径在分析/验证脚本中允许使用绝对路径；可分发的软件代码不得写入含用户名的路径。

## 入口文件

- 当前投稿要求：`journal.md`
- 文件级登记：`项目文件记录.md`
- 变更记录：`CHANGELOG.md`
- 投稿阻断项：`manuscript/SUBMISSION_CHECKLIST.md`
- 软件状态：`docs/08_status.md`

## 待确认事项

- Bioinformatics Application Note 的当前版官方模板与页数限制无法自动获取时，必须请求用户提供指南原文。
- 软件 1.3.0 的 GitHub Release 尚未发布。
- R 包缺陷版本、issue 回复和 2022 年论文影响评估不在本仓库内完成。
- 通讯作者单位邮箱与基金适用范围由作者确认。
