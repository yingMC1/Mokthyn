#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <process.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>
#include <vector>

#include "GameTypes.h"
#include "GameUpdate.h"

namespace mokthyn {
namespace updateDetail {
// 后台线程只写这段固定缓冲区，写完后用 release 语义发布状态；主线程先 acquire 状态再读缓冲区，
// 全程没有跨线程的堆对象，避免静态链接 CRT 下线程退出时析构顺序的问题
inline std::atomic<int> updateState{static_cast<int>(UpdateStatus::Idle)};
inline char latestVersionBuffer[24] = {};
inline bool threadStarted = false;

inline std::string wideToAscii(const std::wstring& wide) {
    if (wide.empty()) return {};
    const int length =
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1) return {};
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, result.data(), length, nullptr, nullptr);
    result.resize(static_cast<size_t>(length - 1));
    return result;
}

// 严格校验 "x.y.z"（三段、每段 1-3 位数字）。免费主机有时会往响应里插广告或时间戳，
// 不收紧格式就会把 "2026.11.07" 这类日期误当成版本号
inline bool isVersionLike(const std::string& text) {
    if (text.empty()) return false;
    int dots = 0;
    int digits = 0;
    for (const char character : text) {
        if (character == '.') {
            if (digits < 1 || digits > 3) return false;
            digits = 0;
            ++dots;
        } else if (character >= '0' && character <= '9') {
            if (++digits > 3) return false;
        } else {
            return false;
        }
    }
    return dots == 2 && digits >= 1;
}

inline void parseVersion(const std::string& text, int out[3]) {
    out[0] = out[1] = out[2] = 0;
    int index = 0;
    int value = 0;
    bool hasDigit = false;
    for (const char character : text) {
        if (character == '.') {
            if (index < 3) out[index] = value;
            ++index;
            value = 0;
            hasDigit = false;
        } else if (character >= '0' && character <= '9') {
            value = value * 10 + (character - '0');
            hasDigit = true;
        }
    }
    if (hasDigit && index < 3) out[index] = value;
}

inline bool newerThanCurrent(const std::string& latest) {
    int latestParts[3] = {};
    int currentParts[3] = {};
    parseVersion(latest, latestParts);
    parseVersion(wideToAscii(GameVersion), currentParts);
    for (int i = 0; i < 3; ++i)
        if (latestParts[i] != currentParts[i]) return latestParts[i] > currentParts[i];
    return false;
}

// 正常接口只返回 "0.1.3" 这样的极短文本，所以：先看整体是否就是版本号；
// 响应里带 BOM 或多余空白时再退化为全文扫描，但只在响应足够短时才扫，
// 长响应一律视为异常（主机插入的广告页），宁可不提示也不报错版本号
inline std::string locateVersion(const std::string& raw) {
    size_t begin = raw.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return {};
    const size_t end = raw.find_last_not_of(" \t\r\n");
    const std::string trimmed = raw.substr(begin, end - begin + 1);
    if (isVersionLike(trimmed)) return trimmed;
    if (trimmed.size() > 200) return {};
    for (size_t i = 0; i < trimmed.size(); ++i) {
        if (trimmed[i] < '0' || trimmed[i] > '9') continue;
        size_t tail = i;
        while (tail < trimmed.size() &&
               (trimmed[tail] == '.' || (trimmed[tail] >= '0' && trimmed[tail] <= '9')))
            ++tail;
        const std::string candidate = trimmed.substr(i, tail - i);
        if (candidate.size() <= 12 && isVersionLike(candidate)) return candidate;
        i = tail - 1;
    }
    return {};
}

inline std::string readResponse(HINTERNET request) {
    std::string result;
    while (result.size() < 2048) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available) || available == 0) break;
        std::vector<char> buffer(available);
        DWORD received = 0;
        if (!WinHttpReadData(request, buffer.data(), available, &received)) break;
        result.append(buffer.data(), received);
    }
    return result;
}

// 拉取单个源的原始响应文本。secure=true 走 443 的 HTTPS，否则走 80 端口
inline std::string fetchVersionTextFrom(const wchar_t* host, const wchar_t* path, bool secure) {
    std::string result;
    HINTERNET session = WinHttpOpen(L"MokthynUpdate/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr) return result;
    WinHttpSetTimeouts(session, UpdateCheckTimeoutMs, UpdateCheckTimeoutMs, UpdateCheckTimeoutMs,
                       UpdateCheckTimeoutMs);
    HINTERNET connection =
        WinHttpConnect(session, host, secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    if (connection != nullptr) {
        HINTERNET request =
            WinHttpOpenRequest(connection, L"GET", path, nullptr, WINHTTP_NO_REFERER,
                               WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0);
        if (request != nullptr) {
            if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                   WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(request, nullptr)) {
                DWORD statusCode = 0;
                DWORD statusSize = sizeof(statusCode);
                if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize,
                                        WINHTTP_NO_HEADER_INDEX) &&
                    statusCode == 200)
                    result = readResponse(request);
            }
            WinHttpCloseHandle(request);
        }
        WinHttpCloseHandle(connection);
    }
    WinHttpCloseHandle(session);
    return result;
}

// 依次尝试：主站 HTTPS → 主站 HTTP → 仓库镜像。某一源拿到响应但解析不出合规版本号
// （典型是被免费主机的脚本校验页吞掉）就继续下一个源，全部失败才认为连不上
inline std::string fetchRemoteVersion() {
    static const struct {
        const wchar_t* host;
        const wchar_t* path;
        bool secure;
    } sources[] = {
        {UpdateCheckHost, UpdateCheckPath, true},
        {UpdateCheckHost, UpdateCheckPath, false},
        {UpdateMirrorHost, UpdateMirrorPath, true},
    };
    for (const auto& source : sources) {
        const std::string raw = fetchVersionTextFrom(source.host, source.path, source.secure);
        if (raw.empty()) continue;
        const std::string version = locateVersion(raw);
        if (!version.empty()) return version;
    }
    return {};
}

inline unsigned __stdcall checkThread(void*) {
    const std::string version = fetchRemoteVersion();
    const bool found = !version.empty();
    const bool newer = found && newerThanCurrent(version);
    if (found) {
        const size_t length =
            std::min(version.size(), sizeof(latestVersionBuffer) - 1);
        std::memcpy(latestVersionBuffer, version.c_str(), length);
        latestVersionBuffer[length] = '\0';
    }
    updateState.store(static_cast<int>(newer ? UpdateStatus::Available
                                             : (found ? UpdateStatus::UpToDate
                                                      : UpdateStatus::Failed)),
                      std::memory_order_release);
    return 0;
}
} // namespace updateDetail

inline UpdateStatus updateStatus() {
    return static_cast<UpdateStatus>(updateDetail::updateState.load(std::memory_order_acquire));
}

inline const std::wstring& updateLatestVersion() {
    static std::wstring cached;
    static bool built = false;
    if (!built) {
        built = true;
        char buffer[sizeof(updateDetail::latestVersionBuffer)] = {};
        std::memcpy(buffer, updateDetail::latestVersionBuffer, sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';
        const int length = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, nullptr, 0);
        if (length > 1) {
            cached.resize(static_cast<size_t>(length));
            MultiByteToWideChar(CP_UTF8, 0, buffer, -1, cached.data(), length);
            cached.resize(static_cast<size_t>(length - 1));
        }
    }
    return cached;
}

inline void startUpdateCheck() {
    if (updateDetail::threadStarted) return;
    updateDetail::threadStarted = true;
    updateDetail::updateState.store(static_cast<int>(UpdateStatus::Checking),
                                    std::memory_order_relaxed);
    const uintptr_t handle =
        _beginthreadex(nullptr, 0, updateDetail::checkThread, nullptr, 0, nullptr);
    if (handle == 0)
        updateDetail::updateState.store(static_cast<int>(UpdateStatus::Failed),
                                        std::memory_order_release);
    else
        CloseHandle(reinterpret_cast<HANDLE>(handle));
}

inline bool openUpdateDownloadPage() {
    const auto result = reinterpret_cast<INT_PTR>(
        ShellExecuteW(nullptr, L"open", DownloadUrl, nullptr, nullptr, SW_SHOWNORMAL));
    return result > 32;
}
} // namespace mokthyn
