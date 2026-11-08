# Network Programming · C 网络编程实验

[![CI](https://github.com/weeweetan/network_programming/actions/workflows/ci.yml/badge.svg)](https://github.com/weeweetan/network_programming/actions/workflows/ci.yml)

从 TCP / UDP 回显，到 select、epoll、libevent、应用层协议和 TLS。基于《TCP/IP 网络编程》等学习材料整理，保留原章节文件名，使用统一构建和真实本机通信测试，让示例更容易运行、比较和继续修改。

| 从这里开始 | 内容 |
| --- | --- |
| [构建与运行](docs/building.md) | 依赖安装、命令参数、完整运行示例 |
| [全部源文件](docs/catalog.md) | 66 个原始 C 文件的用途、状态与目标名 |
| [协议与 I/O 模型](docs/protocols.md) | 消息边界、网络字节序、背压、半关闭 |
| [测试与限制](docs/testing.md) | 分段收发、异常断连、证书校验与平台范围 |
| [继续开发](CONTRIBUTING.md) | 添加示例、更新索引与提交前检查 |

## 五分钟运行

需要 C11 编译器、CMake 3.20+、Python 3.10+。默认构建还需要 OpenSSL 1.1.1+、libevent 2.1+ 和 pkg-config；[依赖安装说明](docs/building.md)包含 Linux / macOS 命令。

```sh
git clone https://github.com/weeweetan/network_programming.git
cd network_programming
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

先体验回显：

```sh
# 终端一：默认只监听本机 127.0.0.1
./build/bin/tcp-echo-server 7000

# 终端二
./build/bin/tcp-echo-client 127.0.0.1 'Hello, sockets!' 7000
# Received: Hello, sockets!
```

暂时没有 OpenSSL / libevent，也可以先构建基础示例：

```sh
cmake -S . -B build-core -DENABLE_TLS=OFF -DENABLE_LIBEVENT=OFF
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
```

## 学习路线

| 阶段 | 推荐目标 | 重点 |
| --- | --- | --- |
| 建立连接 | `hello-server` / `hello-client` | 地址解析、connect、accept、EOF |
| 可靠收发 | `tcp-echo-server` / `tcp-echo-client` | 短写、分段读取、半关闭 |
| 数据报 | `udp-echo-server` / `udp-timeout-client` | 空数据报、来源过滤、有界重试 |
| 并发 I/O | `select-echo-server`、`epoll-et-server` | 非阻塞、待发送队列、事件重置 |
| 事件库 | `libevent-rot13-server`、`libevent-buffered-rot13-server` | 读写事件、bufferevent、水位 |
| 应用协议 | `vote-server-bin` / `vote-server-text` | 长度前缀、分隔符、64 位编码 |
| 线程协作 | `chat-server` / `chat-client` | 参数生命周期、广播、线程回收 |
| TLS | `tls-client` | 信任链、DNS/IP 名称校验、握手期限 |

根目录的章节入口调用 [common](docs/catalog.md) 中的公共实现，避免不同章节反复维护同一处收发错误。Linux 提供 34 个运行目标，macOS 自动排除 3 个 epoll 目标；Windows 可通过 WSL 使用 Linux 构建。

## 运行约定

服务端统一使用 `PORT [BIND_ADDRESS [LIMIT]]`，启动后输出 `LISTEN 地址 端口`。端口 `0` 由系统分配；默认绑定 `127.0.0.1`，IPv6 示例可显式指定 `::1`。TCP 的 `LIMIT` 按接受的连接计数，UDP 按数据报计数；`0` 表示持续运行。问候服务默认处理一个连接后退出，其他服务默认持续运行，按 Ctrl+C 结束。

单次回显客户端使用 `HOST MESSAGE [PORT]`，省略端口时为 `7000`。交互客户端使用 `HOST PORT`，输入 `q` / `Q` 或标准输入 EOF 结束。算术、聊天室、投票和 TLS 的附加参数见[构建与命令](docs/building.md)。

## 验证范围

CI 在 Linux GCC、Linux Clang 和 macOS Clang 上编译并测试，包含 AddressSanitizer / UndefinedBehaviorSanitizer 检查。完整 Linux 构建有 24 组 CTest 检查，macOS 有 21 组；通信测试只使用本机回环地址和临时端口。

历史地址转换、信号、组播、广播、TCP MD5 与 IP 选项实验仍保留为阅读材料；3 个空白草稿也已明确标注。它们未纳入当前自动构建，请先查看[索引中的平台与状态](docs/catalog.md)。
