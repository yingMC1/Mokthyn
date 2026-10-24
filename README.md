# Mokthyn

![C++](https://img.shields.io/badge/Language-C++17/20-blue?style=flat-square&logo=c%2B%2B)
![EGE](https://img.shields.io/badge/Graphics-EGE-orange?style=flat-square)
![CMake](https://img.shields.io/badge/Build-CMake-red?style=flat-square&logo=cmake)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey?style=flat-square&logo=windows)
[![Stars](https://img.shields.io/github/stars/yingMC1/Mokthyn-public?style=flat-square&logo=github&label=Stars&color=yellow)](https://github.com/yingMC1/Mokthyn-public)
[![Issues](https://img.shields.io/github/issues/yingMC1/Mokthyn-public?style=flat-square&label=Issues&color=green)](https://github.com/yingMC1/Mokthyn-public/issues)

## 🏠 yingMC Studio

欢迎访问 [yingMC 工作室官网](https://mokthyn.rf.gd/) 获取最新资讯和资源。

## 📥 下载 Mokthyn

1. [主下载站](https://mokthyn.rf.gd/download)（推荐，更新及时，包含历史版本与 SHA256 校验）。

## 📖 项目简介

**Mokthyn（莫克廷）** 是一款基于 C++ 与 EGE (Easy Graphics Engine) 图形库开发的「逃离雅科夫模式」射击游戏，核心玩法为「搜、打、撤」。

- **底层技术**：现代 C++ (C++17/20)、自研 UI 框架、Verlet 积分离散物理引擎。
- **构建工具**：CMake + Visual Studio / VSCode。
- **运行平台**：Windows x64（绿色免安装）。
- **开发状态**：项目由 [Infinite-Shooter](https://github.com/yingMC1/Infinite-Shooter) 迁移而来，目前在 [Mokthyn](https://github.com/yingMC1/Mokthyn) 私有主仓库中持续迭代。

## 📕 新手手册

首次游玩请务必查看 **[新手手册](./新手手册.md)**。以下是一些核心机制：

- **操作提示**：主菜单按 `3` 可打开分页操作说明；按 `D` 打开下载页，按方向键翻页。
- **设置面板**：主菜单按 `7` 打开设置。默认界面比例 120%，可用方向键调整窗口与比例，按 `0` 恢复默认。
- **存档机制**：数据保存在 `%APPDATA%\mokthyn\save.dat`。**存档已使用 Windows DPAPI 加密**，并与当前 Windows 用户绑定。旧版明文存档会自动迁移。
- **核心对局**：行动地图为 36×24 格。击败右下区域的 Boss（拥有远程攻击，半血狂暴，击杀奖励 100 金币）后方可撤离。
