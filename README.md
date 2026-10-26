# Mokthyn

本项目保证 AI 的贡献远小于 yingMC1 和 abah-abah-111。

本项目保证所有 AI 的建议和 bug 全都是工作人员自己调的。

![C++](https://img.shields.io/badge/Language-C++26-blue?style=flat-square&logo=c%2B%2B)
![EGE](https://img.shields.io/badge/Graphics-EGE-orange?style=flat-square)
![CMake](https://img.shields.io/badge/Build-CMake-red?style=flat-square&logo=cmake)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey?style=flat-square&logo=windows)
[![Stars](https://img.shields.io/github/stars/yingMC1/Mokthyn-public?style=flat-square&logo=github&label=Stars&color=yellow)](https://github.com/yingMC1/Mokthyn-public)
[![Issues](https://img.shields.io/github/issues/yingMC1/Mokthyn-public?style=flat-square&label=Issues&color=green)](https://github.com/yingMC1/Mokthyn-public/issues)

> 一款用 C++ 和 EGE 写的「逃离雅科夫模式」战术撤离射击游戏，玩法就三个字：搜、打、撤。

## 🏠 yingMC Studio

欢迎访问 [yingMC 工作室官网](https://mokthyn.rf.gd)，最新资讯和资源都在那边。

## 📥 下载

主下载站（推荐，更新及时，包含历史版本与 SHA256 校验）。

官方下载页：https://mokthyn.rf.gd/download

## 这是个什么游戏

Mokthyn 是一款**俯视角战术撤离射击游戏**。你在一片随机生成的 **72×48 格**大地图中搜集弹药和医疗包、消灭普通敌人和区域 Boss，击败所有 Boss 后前往撤离点，保持存活并成功撤离。

核心循环：

```
进入地图 → 搜集物资 → 消灭敌人 → 击败 Boss → 前往撤离点 → 撤离成功
     ↓                                                        ↓
  死亡失败 ←———————————————————————————————————————————————————┘
```

支持**单人 / 局域网合作 PVE / PVP 对战**。项目从 Infinite-Shooter 迁移而来，现在在 Mokthyn 私有主仓库里继续迭代。

## 新手先看手册

完整内容在 **新手手册** 里，这里挑几条最要紧的：

**运行**：Windows 10/11 64 位，解压到有写入权限的目录（别放 `C:\Program Files`），双击 `Mokthyn.exe` 启动。首次启动会在 `%APPDATA%\mokthyn` 自动建存档目录。

**操作**：`WASD` 移动，方向键射击/挥刀，`F` 切换枪/刀，`R` 换弹，`E` 拾取，`H` 治疗，`K` 穿脱护甲，`Q` 撤离，`P` 暂停，`ESC` 回主菜单。

**玩法循环**：
1. **搜**：弹药箱给 8~16 发，医疗包给 1 个，走上去按 `E`。
2. **打**：枪 15 伤害/发、弹匣 10 发、射程 9 格；刀 12 伤害、范围 2 格、无需弹药。普通敌人 45 HP，Boss 300 HP、半血狂暴。
3. **撤**：所有 Boss 击败后，站到右下角绿色 `E` 撤离点按 `Q`，保持 5 秒不移动不被攻击。

**难度**（主菜单按 `0` 切换，默认简单）：

| 难度 | 普通敌人 | Boss | 弹药箱 | 医疗包 |
|------|---------|------|--------|--------|
| 简单（默认） | 14 | 1 | 14 | 8 |
| 普通 | 24 | 2 | 24 | 14 |
| 困难 | 33 | 2 | 33 | 19 |

**几条经验**：Boss 远程攻击可被墙壁挡住，绕墙打回合制；普通敌人近战，保持 2 格以上距离；治疗要 3 秒不被打断，别在敌人视线内治；安全区域按 `K` 脱护甲省耐久；死亡后弹药、医疗包、护甲全部清零。

**联机**：UDP 8888 端口，主机权威，主机每 200ms 广播状态。创建主机按 `4`，加入按 `5` 输 IP。PVP 按 `9`，双方初始 50 发备弹、无护甲，地图物资竞争拾取，无法撤离，战斗至最后一人。

## 最近在折腾什么

- 从 Infinite-Shooter 迁过来之后一直在私有主仓库迭代
- C++ 标准往 26 推，CMake 配置也在跟着调
- 道具收集逻辑增强过，加了网络同步支持
- 补了 EGE 的 LGPL-2.1 许可证归属

## 许可证

按仓库里 `LICENSE` 来，里面也写明了 EGE（LGPL-2.1）的归属，用和分发的时候注意遵守。

## 链接

- 仓库：https://github.com/yingMC1/Mokthyn
- 官方下载页：https://mokthyn.rf.gd/download
- yingMC Studio 官网：见仓库内链接
