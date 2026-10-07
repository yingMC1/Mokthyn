**有 bug，而且有几个是发布前必须修的。** 我没有只靠读代码猜，而是用你项目自带的 MinGW 工具链把 release 版编译链接了一遍，另外写了个测试程序**直接调用你项目里真实的存档读写和联机收发包函数**，跑了 9 组共 54 项检查：**39 项通过，15 项失败**，归为 8 个真实缺陷。

你的 `%APPDATA%\mokthyn\save.dat` 全程没被碰过（测试里把 APPDATA 临时重定向了），源码和 `build\` 里的产物也一个字节没改。

---

## 🔴 一、会毁玩家数据

### 1. 开一局联机，单人存档直接没了

[GameLogicImpl.h:122](game.h/GameLogicImpl.h#L122) 在网络模式下把 `hasSavedRun` 置为 false，紧接着 [:129](game.h/GameLogicImpl.h#L129) 就存档；加上 [Mokthyn.cpp:34-36](Mokthyn.cpp#L34-L36) 每 2 秒自动保存，联机地图和敌人会覆盖掉 `save.dat`。

实测：单人档（可继续标记 = 1，生命 55）→ `startRun()` 之后 → 标记变成 **0**、生命变成 **100**，主菜单的「6 继续存档」永久消失。

> 修：`networkActive` 为真时不要写这个存档槽（或另开一个文件）。

### 2. 存档写入端能写出自己读不回来的档

[GameArchiveImpl.h:178](game.h/GameArchiveImpl.h#L178) 原样写 `enemies.size()`，[:101-102](game.h/GameArchiveImpl.h#L101-L102) 读的时候却要求坐标 1..22、[:91](game.h/GameArchiveImpl.h#L91) 要求敌人数 ≤ 9；而客户端从网络包收敌人时上限是 30（[GameNetworkImpl.h:247](game.h/GameNetworkImpl.h#L247)）、坐标允许 0（[:268](game.h/GameNetworkImpl.h#L268)）。

实测：写进 30 个敌人 → `loadArchive()` 返回 false；一个 x=0 的敌人 → 同一个档也读不回来。更糟的是 [:61-78](game.h/GameArchiveImpl.h#L61-L78) 一旦有任何字段不合法就**整个文件作废**，语言、界面比例、窗口尺寸、进度一起丢。

> 触发条件是收到非标准联机端的数据包（正常队友只发 ≤9 个敌人），但这是个定时炸弹——以后你把敌人上限调到 12，所有人的存档立刻报废。修法很简单：写入前夹紧到读取端允许的范围。

---

## 🔴 二、联机一丢包就废

### 3. 握手丢一个包 → 永远连不上

[GameNetworkImpl.h:162](game.h/GameNetworkImpl.h#L162) 主机只在 `!peerConnected` 时才回 ACK。ACK 一旦丢包，客户端每 500ms 重发 CONNECT（[:140-145](game.h/GameNetworkImpl.h#L140-L145)）主机全部无视，双方永远卡住。

实测：第一次 CONNECT 收到 1 个 ACK；之后重发 4 次 CONNECT，**收到 0 个 ACK**。

### 4. 开局丢一个包 → 队友永远卡在大厅

[GameNetworkImpl.h:235](game.h/GameNetworkImpl.h#L235) 只有收到 `START` 才进游戏，而 [:237-286](game.h/GameNetworkImpl.h#L237-L286) 的 STATE 分支完全没有画面切换；`START` 在 [GameLogicImpl.h:131](game.h/GameLogicImpl.h#L131) 只发一次。

实测：只收到 STATE 时，客户端地图和敌人已经同步好了（`totalEnemies` 变成 9），但 `screen` **仍然停在 JoinLobby**；补发 START 才进 Playing。另外 [:307-313](game.h/GameNetworkImpl.h#L307-L313) 的 5 秒断线检测只在 Playing 生效，卡在大厅时连超时都没有。

> 修：把「收到 STATE 就自动进入 Playing」当成连接确认，顺手把上面第 3 条也一起解决。

---

## 🟠 三、状态机漏洞（能被玩家钻空子）

### 5. 治疗/撤离计时器不随离开房间重置 → 菜单里瞬间通关

[Mokthyn.cpp:207-210](Mokthyn.cpp#L207-L210) 暂停按 ESC 回菜单、[:43-46](Mokthyn.cpp#L43-L46) 按 6 继续，两条路都只改 `screen`，从不清 `healing`/`extracting`。

实测：4 项检查全失败——离开房间后两个标志仍是 true，计时器早已过期，所以再进游戏时 [GameLogicImpl.h:393](game.h/GameLogicImpl.h#L393)（5 秒撤离）和 [:378](game.h/GameLogicImpl.h#L378)（3 秒治疗）立刻成立。

**在撤离点按 Q → ESC 回菜单 → 等 5 秒 → 按 6 = 直接胜利**；治疗同理，浪费一个医疗包就能白拿 20 血，零风险。

### 6. 主机的撤离状态会劫持客户端 → 客户端瞬间"胜利"

[GameNetworkImpl.h:261](game.h/GameNetworkImpl.h#L261) 把「主机在撤离」和「队友在撤离」合并进客户端自己的 `extracting`，于是主机的进度会去驱动客户端的本地计时器。

实测：只有主机在撤离时，客户端的 `extracting` 被点亮，而它的 `extractStarted` 还停在初始值（差了 **283487 秒**）→ 5 秒判定当场满足。主机刚按 Q 客户端就弹胜利，主机要是被打断，两边就彻底不同步了。

### 7. 队友护甲耐久永远不会掉

[GameLogicImpl.h:227-231](game.h/GameLogicImpl.h#L227-L231) 主机扣 `peerArmorDurability`，但客户端每 100ms 发一次 POS，[GameNetworkImpl.h:195](game.h/GameNetworkImpl.h#L195) 又把它恢复成客户端那个（客户端自己从不扣的）数值。

实测：75 →（主机挨打）71 → 下一个 POS → **75**。等于队友四级甲永久免伤。

---

## 🟡 四、体验 / 小问题

8. **失败提示玩家根本看不到**：[Mokthyn.cpp:56,58](Mokthyn.cpp#L56-L58) 按 4/5 失败、[:147](Mokthyn.cpp#L147) 下载页打开失败都走 `setMessage`，而 `message` 只在 [GameRenderImpl.h:405-406](game.h/GameRenderImpl.h#L405-L406) 的战斗 HUD 里绘制，主菜单只画 `archiveStatus`（[:427](game.h/GameRenderImpl.h#L427)）。**端口 8888 被占用时按 4 会毫无反应**——这大概率是发布后你收到的第一条反馈。
9. 敌人会斜穿墙角：[GameLogicImpl.h:209-210](game.h/GameLogicImpl.h#L209-L210) 方向表第一项是 `{dx, dy}`。
10. [GameArchiveImpl.h:33](game.h/GameArchiveImpl.h#L33) 连续调用两次 `GetLastError()`，实践上没事但很脆。
11. [Mokthyn.cpp:137](Mokthyn.cpp#L137) `ESC || ENTER` 短路，可能把 ENTER 留到下一帧，在主菜单里误开一局。
12. 还没输入 IP 时 [GameNetworkImpl.h:140-145](game.h/GameNetworkImpl.h#L140-L145) 每 500ms 往 `0.0.0.0:0` 发 CONNECT，发送失败也不处理。
13. `ege.h:5316` 有 `#define Sleep(ms) delay_ms(ms)`——在游戏代码里写 `Sleep` 会变成必须在 `initgraph` 之后才能调的 EGE 延时（我的测试程序就是这么崩的）。你现在没用，别踩。

---
