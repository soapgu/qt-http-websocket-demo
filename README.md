# Qt HTTP + WebSocket Demo

演示 **cpp-httplib 服务端**与 **Qt 官方 HTTP、WebSocket 客户端**互通，为 Qt 程序内嵌轻量网络服务提供参考。

> 当前阶段：服务端已实现 HTTP 状态查询与 JSON 回显，启动后持续监听。客户端已提供 Qt Quick 调试窗口；已支持同端口 WebSocket 广播及信号正常停止，支持 HTTP 提交和 WS 订阅。

## 技术方案

| 角色 | 技术 | 用途 |
| --- | --- | --- |
| 服务端 | cpp-httplib | 提供 HTTP API 和 WebSocket 服务 |
| HTTP 客户端 | Qt Network：`QNetworkAccessManager` | 查询状态、发送请求 |
| WebSocket 客户端 | Qt WebSockets：`QWebSocket` | 接收 HTTP 触发的广播 |
| 构建工具 | CMake | 管理服务端与 Qt 客户端构建 |

```text
Qt 客户端                              服务端
QNetworkAccessManager ── HTTP ──→ cpp-httplib
QWebSocket            ←── WS ───→ cpp-httplib
                                  共用一个监听端口
```

双方通过标准 HTTP / WebSocket 协议通信，无需使用相同的网络库。服务端可以独立运行，也可以在后续集成进 Qt 程序。

## 项目结构

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
    ├── Main.qml            # Qt Quick 调试窗口
    └── src/               # Qt HTTP / WebSocket 客户端实现
```

已支持统一构建及按需选择客户端或服务端构建。子项目通过顶层工程配置，不支持直接单独配置子目录。运行时先启动服务端，再启动客户端连接。

- **服务端 `server/`**：负责监听端口、HTTP 路由和 WebSocket 连接处理，独立运行，不依赖 Qt。
- **客户端 `client/`**：负责通过 Qt 官方模块发起 HTTP 请求、建立 WebSocket 连接并展示通信结果。
- **顶层工程**：负责依赖管理、子项目选择与统一构建。

若后续需要在现有 Qt 程序中内嵌服务，可将服务端核心逻辑封装成库，由 Qt 主程序启动，并保留独立服务端用于调试。

## 构建与运行

需要 CMake 3.21 及以上、支持 C++17 的编译器。构建客户端还需要 Qt 6.8.3 及以上版本的 Core、Gui、Qml、Quick、QuickControls2、Network、WebSockets 模块；启用服务端时由 CMake 自动获取 cpp-httplib 和 nlohmann/json。

以下命令在仓库根目录执行。将 `CMAKE_PREFIX_PATH` 替换为本机 Qt 安装前缀（例如 macOS 的 `/Users/guhui/Qt/6.8.3/macos`）。

统一构建（两个开关默认均为 `ON`）：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/你的/Qt/安装前缀"
cmake --build build --config Debug
./build/bin/qt_http_server
# 在另一个终端运行客户端
./build/bin/qt_http_client
```

仅构建服务端（不查找或依赖 Qt）：

```bash
cmake -S . -B build-server -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF
cmake --build build-server --config Debug
./build-server/bin/qt_http_server
```

仅构建客户端：

```bash
cmake -S . -B build-client -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON \
  -DCMAKE_PREFIX_PATH="/你的/Qt/安装前缀"
cmake --build build-client --config Debug
./build-client/bin/qt_http_client
```

可执行程序统一输出到对应构建目录的 `bin/`，多配置生成器也使用该目录；Windows 下程序名带 `.exe` 后缀。两个开关同时为 `OFF` 时配置会明确报错。

服务端启动后持续监听 `127.0.0.1:8080`，通过 Qt Creator 停止按钮或终端 Ctrl+C 终止。客户端显示调试窗口，先连接 WebSocket，再提交 HTTP JSON。关闭窗口会取消请求并关闭连接，最多等待 2 秒。Ctrl+C（SIGINT）或 SIGTERM 会停止监听、关闭连接并等待线程回收；强制结束进程（如 SIGKILL）无法保证清理。

### 服务端依赖与离线构建

服务端使用 [cpp-httplib v0.66.0](https://github.com/yhirose/cpp-httplib/tree/v0.66.0)，遵循 [MIT 许可证](https://github.com/yhirose/cpp-httplib/blob/v0.66.0/LICENSE)。归档 SHA256 为 `537a77cdb96d3b4ad64d0b7103971de52eac75cd4ee03a2fdb84417b0d742459`。

JSON 解析使用 [nlohmann/json v3.11.3](https://github.com/nlohmann/json/tree/v3.11.3)，遵循 [MIT 许可证](https://github.com/nlohmann/json/blob/v3.11.3/LICENSE.MIT)，使用上游发布的 `json.tar.xz` 归档，SHA256 为 `d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d`，不加载上游构建工程。

首次配置服务端需要联网下载；后续配置复用同一构建目录中的下载缓存。仅构建客户端不会下载或配置该依赖。当前仅使用头文件与系统线程库，不加载上游 CMake 工程，不启用 TLS、压缩、示例、测试或安装功能。

如需离线构建，预先下载并解压同版本源码，再指定包含 `httplib.h` 的绝对目录：

```bash
cmake -S . -B build-offline -DBUILD_CLIENT=OFF \
  -DFETCHCONTENT_SOURCE_DIR_CPP_HTTPLIB="/本地/cpp-httplib-0.66.0" \
  -DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON="/本地/json-3.11.3"
cmake --build build-offline
```

本地源码覆盖不会执行归档摘要校验，请确保目录来自上述固定版本。下载或校验失败会由 CMake 报错，不自动切换版本或下载来源。

## 选型背景

Qt 官方提供 `Qt HTTP Server + Qt WebSockets` 服务端组合。本项目沿用前期讨论，选择 cpp-httplib 提供服务端，Qt 官方模块提供客户端，减少服务端与 Qt 的耦合。

- cpp-httplib 使用 MIT 许可证，采用单头文件方式集成。
- Qt 客户端网络使用 Network 和 WebSockets，界面使用 Qt Quick；不依赖 Qt HTTP Server。
- 前期项目使用 Qt 6.8.3，本 Demo 计划以该版本为基线；构建前需安装对应的 Network、WebSockets 模块。
- 服务端固定使用 cpp-httplib v0.66.0，通过 FetchContent 下载并校验 SHA256，不跟随主分支。

## 接口

固定监听地址为 `127.0.0.1:8080`，用于本机调试。

| 协议 | 地址 | 行为 |
| --- | --- | --- |
| HTTP | `GET /api/status` | 已实现：200，`{"status":"ok"}` |
| HTTP | `POST /api/echo` | 已实现：200，`{"data":请求JSON值}` |
| WebSocket | `/ws` | 已实现：订阅 /api/echo 成功提交的 JSON 广播 |

### HTTP 调试

先启动服务端，再在另一个终端执行：

```bash
curl -i http://127.0.0.1:8080/api/status
curl -i http://127.0.0.1:8080/api/echo \
  -H 'Content-Type: application/json' --data-binary '{"message":"你好"}'
curl -i http://127.0.0.1:8080/api/echo --data-binary 'invalid'
```

成功响应的 Content-Type 为 `application/json`。回显接受所有 JSON 值（对象、数组、字符串、数字、布尔值和 null），保留值的含义，不保留原始排版。不强制校验请求 Content-Type。

空请求体、非法 JSON 或非法 UTF-8 返回 HTTP 400 和 `{"error":"请求体必须是合法 JSON"}`。端口被占用时，服务端输出绑定失败原因并以非零状态退出。

### WebSocket 调试与停止

连接 `ws://127.0.0.1:8080/ws` 后，只接收广播，不发送业务消息。每次 `/api/echo` 成功提交，所有当前订阅者都会收到请求 JSON 本身（文本帧），HTTP 仍返回 `{"data":请求JSON值}`。非法 JSON 不广播，无订阅者时 HTTP 仍成功，不缓存历史消息。WS 客户端发来的业务消息被忽略，协议控制帧正常处理。

客户端 HTTP 区域支持状态查询和 JSON 提交，展示状态码、正文和错误；一次处理一个请求，超时 5 秒。WS 区域展示连接状态及广播日志（最多 500 条），连接超时 5 秒，不自动重连。多个客户端连接后，可通过任意窗口或 curl 提交来观察广播。

每个订阅者队列上限 100 条，队列溢出时关闭慢连接，不影响 HTTP 和其他订阅者。不保证并发请求的广播顺序，停止时不保证排空队列。
在服务端终端按 Ctrl+C，或向进程发送 SIGTERM，等待“服务已停止，连接与线程已回收”提示。停止期间 WebSocket 发送 GoingAway（1001）关闭帧；对端不回应关闭帧时最多等待 1 秒。每个连接每 200 毫秒检查停止状态，HTTP 读写和空闲连接超时为 2 秒。Qt Creator 若直接强制结束进程，无法保证执行正常停止流程。

## 线程与生命周期

- cpp-httplib 使用阻塞 I/O；嵌入 Qt 程序时，监听和网络处理应放在独立线程，避免阻塞界面事件循环。
- 网络回调通过排队信号槽或 `QMetaObject::invokeMethod` 与 Qt 对象交互，不直接操作界面。
- 服务端退出时停止监听、关闭连接并等待工作线程结束；客户端退出时关闭自身连接、释放网络对象。
- 当前上游 WebSocket 实现为每个连接分配处理线程及心跳线程，适合少量连接的演示和控制场景；本项目尚未进行并发、内存或性能测试。

## 后续实现

- [x] **构建工程**：创建顶层 CMake 工程及 `server/`、`client/` 子项目，支持统一构建和按需选择构建。
- [x] **服务端**：固定 cpp-httplib 版本并接入服务端 CMake 配置。
- [x] **服务端**：实现 HTTP 状态与回显接口。
- [x] **服务端**：实现同端口 WebSocket 广播。
- [x] **服务端**：实现停止监听、连接关闭与线程回收。
- [x] **客户端**：接入 Qt Network、Qt WebSockets 模块，实现 HTTP 请求与响应展示。
- [x] **客户端**：实现 WebSocket 连接、广播接收、断开及错误展示。
- [x] **客户端**：实现退出时的连接关闭与网络对象清理。
- [x] **客户端与服务端联调**：验证 HTTP 提交、WS 广播接收、断开和双方程序退出。
- [x] **文档**：分别补充客户端、服务端的实际构建与运行步骤，以及依赖许可证说明。

## 联调与验收

需要 Python 3（仅标准库）及已构建的服务端。测试固定使用本机 8080 端口，运行前请停止手动启动的服务端；脚本只清理自己启动的进程。

```bash
# 核心 HTTP / WS 检查
python3 tests/integration.py --server "$PWD/build/bin/qt_http_server"

# 同时验证真实客户端 C++ 网络控制对象（不依赖窗口操作）
cmake -S tests -B build-tests -DCMAKE_PREFIX_PATH="/你的/Qt/安装前缀"
cmake --build build-tests --config Debug
python3 tests/integration.py --server "$PWD/build/bin/qt_http_server" \
  --client-smoke "$PWD/build-tests/client_smoke"
```

Windows 请使用相应的 `.exe` 路径；多配置生成器的测试程序通常位于 `build-tests/Debug/`。测试覆盖无订阅者提交、多个订阅者广播、非法 JSON、禁止 WS 回显、重连、SIGINT/SIGTERM 停止和端口重新绑定；客户端检查包含 HTTP、错误正文、连接失败、超时及退出。信号测试面向 macOS/Linux。

### 手动窗口验收

1. 启动服务端，在两个终端分别启动客户端。
2. 两个窗口分别点击“连接”，确认 WS 状态变为“已连接”。
3. 任一窗口点击“查询状态”，应显示 HTTP 200 和 `{"status":"ok"}`。
4. 在任一窗口提交 `{"message":"你好"}`，HTTP 响应为 `{"data":{"message":"你好"}}`，两个窗口的接收日志都显示请求 JSON。
5. 提交非法 JSON，应显示 400 正文且无新广播；断开一个窗口后再次提交，另一个窗口仍收到广播。
6. 关闭客户端窗口；服务端用 Ctrl+C 停止，其他客户端应显示断开。

自动检查不等于视觉验收：QML 加载可通过启动检查确认，窗口布局、缩放和真实点击交互仍待人工检查。

### Qt Creator 使用

打开顶层 `CMakeLists.txt`，选择 Qt 6.8.3 对应 Kit，先点击锤子构建。运行目标选择 `qt_http_server` 后启动，再切换 `qt_http_client` 启动客户端。若提示找不到可执行文件，请先构建当前 Kit 的目录；命令行的 `build/` 与 Creator 的构建目录可能不同。

### 下载代理及排障

仅首次下载或重新配置依赖时需要网络，可在终端配置代理：

```bash
export http_proxy=http://127.0.0.1:7897
export https_proxy=http://127.0.0.1:7897
export no_proxy=127.0.0.1,localhost
cmake -S . -B build -DCMAKE_PREFIX_PATH="/你的/Qt/安装前缀"
cmake --build build
```

Qt Creator 请在“项目 → 构建设置”的构建/配置环境中添加同样的变量。代理地址属于本机设置，不写死在工程中；客户端网络对象直接连接，不使用下载代理。

- 找不到 `Qt6WebSockets`：通过 Qt 官方维护工具补装与 Kit 版本、架构相同的 WebSockets；不要混用其他 Qt 版本的模块。
- 找不到 QML 模块或界面组件：确认 Kit 包含 Qt Declarative、Quick、Quick Controls，再重新配置。
- 端口占用：停止已经运行的服务端，确认本机 8080 端口释放后再启动。
- 客户端连接失败：先确认服务端已启动，HTTP 和 WS 地址分别使用 `http://` 与 `ws://`。

## 依赖许可证

| 依赖 | 版本 | 授权与来源 |
| --- | --- | --- |
| cpp-httplib | v0.66.0 | [MIT](https://github.com/yhirose/cpp-httplib/blob/v0.66.0/LICENSE) |
| nlohmann/json | v3.11.3 | [MIT](https://github.com/nlohmann/json/blob/v3.11.3/LICENSE.MIT) |
| Qt Core、Gui、Network | ≥ 6.8.3 | 商业或开源授权，见 [Qt 授权](https://doc.qt.io/qt-6.8/licensing.html) |
| Qt Qml、Quick、Quick Controls | ≥ 6.8.3 | 商业或开源授权，见 [Qt Quick](https://doc.qt.io/qt-6.8/qtquick-index.html)、[Quick Controls](https://doc.qt.io/qt-6.8/qtquickcontrols-index.html) |
| Qt WebSockets | ≥ 6.8.3 | 商业或开源授权，见 [模块授权](https://doc.qt.io/qt-6.8/qtwebsockets-index.html) |

本机 Qt 6.8.3 的总许可证文件为 Qt 安装根目录下的 `Licenses/LICENSE`；本机所用模块公共头文件的 SPDX 声明包含商业、LGPLv3 和 GPL 许可选项。具体使用与分发应遵循所选许可证及随 Qt 提供的第三方许可；本项目不依赖 Qt HTTP Server，不为仓库自动选择许可证。

## 参考资料

- [cpp-httplib 项目](https://github.com/yhirose/cpp-httplib)
- [cpp-httplib WebSocket 文档与线程模型](https://github.com/yhirose/cpp-httplib/blob/master/README-websocket.md)
- [Qt 6.8 QNetworkAccessManager](https://doc.qt.io/qt-6.8/qnetworkaccessmanager.html)
- [Qt 6.8 QWebSocket](https://doc.qt.io/qt-6.8/qwebsocket.html)
- [Qt 6.8 HTTP Server](https://doc.qt.io/qt-6.8/qthttpserver-index.html)

本仓库暂未选择许可证；依赖库分别遵循各自许可证。
