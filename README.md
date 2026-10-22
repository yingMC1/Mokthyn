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

1. [主下载站](https://mokthyn.rf.gd/download)（推荐，更新及时，包含历史版本与 SHA256 校验）
2. [备用下载站 (Maozi)](https://mokthyn-3xya338.maozi.io/)（更新可能滞后）
3. [备用下载站 (Workers)](https://fancy-unit-740c.ying110189.workers.dev)（更新可能滞后）

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

---

## 🛠️ 开发者指南 (Developer Guide)

为了保护核心业务逻辑与美术资产，**Mokthyn 主程序（`.cpp` 源码）当前不公开**，但我们提供了一套强大的**基于头文件（`.h`）的扩展机制**。

如果你希望为 Mokthyn 开发 Mod 或扩展功能，**不需要重新编译整个游戏**。你只需要编写符合规范的 `.h` 文件即可。

### 1. 本地开发环境配置

虽然不需要编译主游戏，但为了测试你写的 `.h` 文件是否有语法错误，你需要配置基础的 C++ 环境：

1. **安装编译器**：推荐安装 [Visual Studio 2022](https://visualstudio.microsoft.com/zh-hans/)（勾选“使用 C++ 的桌面开发”）。
2. **配置 EGE 图形库**：
   - 前往 [EGE 官网](https://xege.org/) 下载适用于 MinGW 或 MSVC 的包。
   - 将 `include` 和 `lib` 目录配置到你的编译器中，确保你能正常跑通一个 Hello World 窗口。
3. **获取接口文件**：下载本仓库中的 `game.h`（或其他公开接口头文件），放入你的项目目录。

### 2. 如何编写扩展头文件？

你的 `.h` 文件将作为插件被游戏主程序动态加载。编写时请注意以下几点：

- **避免依赖 `main` 函数**：你的头文件只是功能的扩展，不要包含自己的 `main()`。
- **使用公开的 API**：你可以通过 `game.h` 中暴露的类来操作 UI、修改物理参数或注入新武器。
- **防重复命名**：请使用命名空间（如 `namespace MyCustomWeapon`）包裹你的代码，避免与主程序冲突。

**示例结构：**
```cpp
#pragma once
#include "game.h"

namespace Mod_NewWeapon {
    // 你的扩展逻辑
    void Init() {
        // 注册新武器...
    }
}
