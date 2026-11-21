#pragma once

namespace mokthyn {
inline bool loadArchive();
inline bool saveArchive();
// 存档脏检测哈希：把会写进存档的状态压成 64 位值，供 saveArchive 跳过无变化写盘；
// 刻意排除敌人攻击计时 nextAttack（落盘时按剩余冷却钳制，读档后会重置）
inline unsigned long long archiveStateHash();
inline void beginNetworkArchiveSession();
inline void restoreNetworkArchiveSession();
}    // namespace mokthyn
