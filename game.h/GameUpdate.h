#pragma once

#include <string>

namespace mokthyn {
// 后台更新检查的状态机：Idle → Checking → UpToDate / Available / Failed
// 检查在独立线程里跑，主线程只读状态，任何情况下都不会卡住启动或掉帧
enum class UpdateStatus : int { Idle, Checking, UpToDate, Available, Failed };

inline UpdateStatus updateStatus();
// 仓库源返回的最新版本号（Available / UpToDate 状态下有意义）
inline const std::wstring &updateLatestVersion();
// 发起一次后台检查。启动时调用一次，设置页「检查更新」可以反复调用；
// 上一轮还没结束时本次请求会被忽略
inline void startUpdateCheck();
// 取走「发现新版本、应该打开下载页」的请求标记。检查线程只置标志，
// 由主线程每帧取一次再真正 ShellExecute，避免后台线程抢焦点
inline bool consumeUpdateOpenRequest();
// 用默认浏览器打开下载页，返回是否成功
inline bool openUpdateDownloadPage();
}    // namespace mokthyn
