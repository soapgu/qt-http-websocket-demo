# Qt HTTP + WebSocket Demo

演示 **cpp-httplib 服务端**与 **Qt 官方 HTTP、WebSocket 客户端**互通，为 Qt 程序内嵌轻量网络服务提供参考。

> 当前阶段：方案与接口规划。尚未实现代码、构建配置或运行示例，以下接口均为计划。

## 技术方案

| 角色 | 技术 | 用途 |
| --- | --- | --- |
| 服务端 | cpp-httplib | 提供 HTTP API 和 WebSocket 服务 |
| HTTP 客户端 | Qt Network：`QNetworkAccessManager` | 查询状态、发送请求 |
| WebSocket 客户端 | Qt WebSockets：`QWebSocket` | 双向消息通信 |
| 构建工具 | CMake | 管理服务端与 Qt 客户端构建 |

```text
Qt 客户端                              服务端
QNetworkAccessManager ── HTTP ──→ cpp-httplib
QWebSocket            ←── WS ───→ cpp-httplib
                                  共用一个监听端口
```

双方通过标准 HTTP / WebSocket 协议通信，无需使用相同的网络库。服务端可以独立运行，也可以在后续集成进 Qt 程序。

## 项目结构（规划）

采用一个仓库、一个顶层 CMake 工程，包含客户端与服务端两个子项目，分别生成独立的可执行程序。

```text
qt-http-websocket-demo/
├── README.md
├── CMakeLists.txt          # 顶层工程，统一管理两个子项目
├── server/
│   ├── CMakeLists.txt      # 服务端构建配置
│   └── src/               # cpp-httplib 服务端实现
└── client/
    ├── CMakeLists.txt      # 客户端构建配置
    └── src/               # Qt HTTP / WebSocket 客户端实现
```

以上目录和构建文件尚未创建。后续支持统一构建，也支持按需选择客户端或服务端构建；运行时先启动服务端，再启动客户端连接。

- **服务端 `server/`**：负责监听端口、HTTP 路由和 WebSocket 连接处理，独立运行，不依赖 Qt。
- **客户端 `client/`**：负责通过 Qt 官方模块发起 HTTP 请求、建立 WebSocket 连接并展示通信结果。
- **顶层工程**：负责依赖管理、子项目选择与统一构建。

若后续需要在现有 Qt 程序中内嵌服务，可将服务端核心逻辑封装成库，由 Qt 主程序启动，并保留独立服务端用于调试。

## 选型背景

Qt 官方提供 `Qt HTTP Server + Qt WebSockets` 服务端组合。本项目沿用前期讨论，选择 cpp-httplib 提供服务端，Qt 官方模块提供客户端，减少服务端与 Qt 的耦合。

- cpp-httplib 使用 MIT 许可证，采用单头文件方式集成。
- Qt 客户端仅需 Network 和 WebSockets 模块；不依赖 Qt HTTP Server。
- 前期项目使用 Qt 6.8.3，本 Demo 计划以该版本为基线；构建前需安装对应的 Network、WebSockets 模块。
- 第三方依赖的具体版本将在实现时固定，不直接跟随主分支。

## 计划演示的接口

默认监听地址计划为 `127.0.0.1:8080`，用于本机调试。

| 协议 | 地址 | 计划行为 |
| --- | --- | --- |
| HTTP | `GET /api/status` | 返回 JSON 服务状态 |
| HTTP | `POST /api/echo` | 回显请求内容 |
| WebSocket | `/ws` | 接收文本并回显，验证双向通信 |

## 线程与生命周期

- cpp-httplib 使用阻塞 I/O；嵌入 Qt 程序时，监听和网络处理应放在独立线程，避免阻塞界面事件循环。
- 网络回调通过排队信号槽或 `QMetaObject::invokeMethod` 与 Qt 对象交互，不直接操作界面。
- 服务端退出时停止监听、关闭连接并等待工作线程结束；客户端退出时关闭自身连接、释放网络对象。
- 当前上游 WebSocket 实现为每个连接分配处理线程及心跳线程，适合少量连接的演示和控制场景；本项目尚未进行并发、内存或性能测试。

## 后续实现

- [ ] **构建工程**：创建顶层 CMake 工程及 `server/`、`client/` 子项目，支持统一构建和按需选择构建。
- [ ] **服务端**：固定 cpp-httplib 版本并接入服务端 CMake 配置。
- [ ] **服务端**：实现 HTTP 状态与回显接口。
- [ ] **服务端**：实现同端口 WebSocket 回显。
- [ ] **服务端**：实现停止监听、连接关闭与线程回收。
- [ ] **客户端**：接入 Qt Network、Qt WebSockets 模块，实现 HTTP 请求与响应展示。
- [ ] **客户端**：实现 WebSocket 连接、消息收发、断开及错误展示。
- [ ] **客户端**：实现退出时的连接关闭与网络对象清理。
- [ ] **客户端与服务端联调**：验证 HTTP 请求、WebSocket 消息收发、断开和双方程序退出。
- [ ] **文档**：分别补充客户端、服务端的实际构建与运行步骤，以及依赖许可证说明。

## 参考资料

- [cpp-httplib 项目](https://github.com/yhirose/cpp-httplib)
- [cpp-httplib WebSocket 文档与线程模型](https://github.com/yhirose/cpp-httplib/blob/master/README-websocket.md)
- [Qt 6.8 QNetworkAccessManager](https://doc.qt.io/qt-6.8/qnetworkaccessmanager.html)
- [Qt 6.8 QWebSocket](https://doc.qt.io/qt-6.8/qwebsocket.html)
- [Qt 6.8 HTTP Server](https://doc.qt.io/qt-6.8/qthttpserver-index.html)

本仓库暂未选择许可证；依赖库分别遵循各自许可证。
