#pragma once

#include <string>

namespace mokthyn {
// 后台更新检查的状态机：Idle → Checking → UpToDate / Available / Failed
// 检查在独立线程里跑，主线程只读状态，任何情况下都不会卡住启动或掉帧
enum class UpdateStatus : int { Idle, Checking, UpToDate, Available, Failed };

inline UpdateStatus updateStatus();
// 服务器返回的最新版本号（仅 Available 状态下有意义）
inline const std::wstring& updateLatestVersion();
// 启动后台检查，重复调用只生效一次
inline void startUpdateCheck();
// 用默认浏览器打开下载页，返回是否成功
inline bool openUpdateDownloadPage();
} // namespace mokthyn
