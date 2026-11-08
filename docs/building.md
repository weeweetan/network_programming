# 构建与运行

## 环境准备

支持 Linux、macOS 的 POSIX socket / pthread 环境；Windows 请使用 WSL。需要 C11 编译器、CMake 3.20+，测试需要 Python 3.10+。

Debian / Ubuntu：

```sh
sudo apt-get update
sudo apt-get install build-essential cmake python3 pkg-config libevent-dev libssl-dev openssl
```

macOS：先安装 Xcode Command Line Tools，再安装依赖。

```sh
xcode-select --install
brew install cmake python pkgconf libevent openssl@3
```

Apple Silicon 请使用与编译器架构一致的 Homebrew 依赖；arm64 程序不能链接 x86_64 的 OpenSSL / libevent。可通过 `file` 检查库文件架构。配置时可显式指定 Homebrew 前缀：

```sh
PKG_CONFIG_PATH="$(brew --prefix libevent)/lib/pkgconfig" \
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

默认开启 `ENABLE_TLS` 和 `ENABLE_LIBEVENT`，缺失依赖会在配置时明确失败。只学习基础 socket 时可同时关闭这两个选项；相应目标和测试会自动排除。生成文件放在 `build*/`，不提交到 Git。

## 目标与参数

下表中的程序位于 `build/bin/`，源码入口见[完整索引](catalog.md)。服务端端口为纯数字 `0..65535`；客户端为 `1..65535`。

| 目标 | 参数 | 行为 |
| --- | --- | --- |
| TCP、UDP、select、epoll、libevent、chat、op、vote 服务端 | `PORT [BIND_ADDRESS [LIMIT]]` | 默认 `127.0.0.1`，`LIMIT=0` 持续运行 |
| `hello-server`、`tcp-server` | 同上 | 未显式指定 LIMIT 时处理 1 个连接 |
| `tcp-echo-client`、`tcp-echo-client4` | `HOST MESSAGE [PORT]` | 一次消息；默认端口 7000 |
| `udp-echo-client`、`udp-timeout-client` | `HOST MESSAGE [PORT]` | 一次数据报；后者最多 5 次尝试 |
| `echo-client`、`echo-client2`、`stdio-echo-client`、`uecho-client` | `HOST PORT` | 标准输入交互；q / Q 或 EOF 退出 |
| `hello-client`、`tcp-client` | `HOST PORT` | 读取问候直到 EOF |
| `chat-client` | `HOST PORT NAME` | NAME 为 1..16 字节，禁止换行 |
| `op-client` | `HOST PORT` | 逐行输入数量、操作数和运算符 |
| `vote-client-bin`、`vote-client-text` | `HOST PORT CANDIDATE [I]` | 候选编号 0..1000，I 表示查询 |
| `tls-client` | `HOST PORT [CA_FILE [SERVER_NAME]]` | 校验信任链与连接名，输出 HTTP 响应 |

`LIMIT` 范围为 `0..1000000`，TCP 按接受连接数计，UDP 按接收数据报数计。并发服务到达 LIMIT 后停止接受连接，已连接客户端结束后退出。服务端启动时输出 `LISTEN 地址 端口`；传入端口 `0` 时从这行取得系统分配的端口，避免测试占用固定端口。

默认绑定本机；需要其他接口时显式传入地址，例如 `0.0.0.0` 或 `::1`。IPv4 专用入口 `tcp-echo-server4`、`echo-server`、`stdio-echo-server`、问候服务只接受 IPv4 地址，其余通用入口依据绑定地址选择地址族。

## 示例

每组先在一个终端启动服务，再在另一个终端运行客户端。示例端口不需要 root 权限；用完 Ctrl+C 关闭服务。

```sh
# UDP：支持零长度数据报，超时客户端会重发
./build/bin/udp-echo-server 7001
./build/bin/udp-timeout-client 127.0.0.1 'hello udp' 7001

# select：另一连接空闲时仍能回显
./build/bin/select-echo-server 7002
./build/bin/echo-client 127.0.0.1 7002

# ROT13：输入 Hello，输出 Uryyb
./build/bin/libevent-buffered-rot13-server 7003
./build/bin/echo-client 127.0.0.1 7003

# 二进制投票：一票后查询；服务重启后计数清零
./build/bin/vote-server-bin 7004
./build/bin/vote-client-bin 127.0.0.1 7004 23
./build/bin/vote-client-bin 127.0.0.1 7004 23 I

# 文本版本必须配套使用 text 客户端
./build/bin/vote-server-text 7005
./build/bin/vote-client-text 127.0.0.1 7005 23

# 聊天室：可以在多个终端使用不同名字连接
./build/bin/chat-server 7006
./build/bin/chat-client 127.0.0.1 7006 alice

# 算术：数量 3、操作数 4/5/6、加法，结果 15
./build/bin/op-server 7007
printf '3\n4\n5\n6\n+\n' | ./build/bin/op-client 127.0.0.1 7007
```

Linux 上可以将 select 目标替换为 `epoll-echo-server`、`epoll-lt-server` 或 `epoll-et-server` 比较事件模型。ROT13 只转换 ASCII 英文字母，其他字节保持原样；现在按字节流返回，不必等待完整行。

`stdio-echo-server` 演示独立读写 FILE 流，每遇换行、累计 1024 字节或读取 EOF 时刷新输出；交互客户端对最后一个没有换行的输入先半关闭，再读取尾部响应。`echo-client2`、`stdio-echo-client` 保留章节入口，客户端复用完整收发实现。

## TLS

```sh
# 使用系统信任根与 HOST 名称校验（需要自行提供可访问的 HTTPS 主机）
./build/bin/tls-client example.com 443

# 本地证书：连接地址、信任根与证书名称分别指定
./build/bin/tls-client 127.0.0.1 8443 ./ca.pem localhost
```

TLS 最低版本为 1.2；DNS 名称设置 SNI 并验证主机名，IP 地址验证证书的 IP SAN。CA_FILE 指定信任文件；未指定时使用 OpenSSL 默认信任路径。SERVER_NAME 同时决定证书名称校验和 HTTP Host，不能包含换行。证书不可信、名称不符、握手超时或响应被截断都以非零状态退出。

客户端发送 `GET / HTTP/1.1` 与 `Connection: close`，不发送 PROXY 前导或固定 Cookie；stdout 是原始 HTTP 响应，状态信息写入 stderr。它是有期限的 TLS/HTTP 传输示例，未实现重定向、解压或完整 HTTP 解析。

## 故障排查

| 现象 | 处理 |
| --- | --- |
| CMake 找不到 OpenSSL / libevent | 安装开发包，或关闭相应选项；macOS 检查前缀和架构 |
| `Address already in use` | 换端口；自动化测试可传 0 获取临时端口 |
| `Connection refused` | 检查服务已启动、端口正确、IPv4 / IPv6 地址族一致 |
| `Message too long` | UDP 受到协议和系统限制，缩小消息；macOS 默认上限可能更小 |
| 证书校验失败 | 检查 CA_FILE、证书有效期以及 SERVER_NAME 对应的 SAN |
| 单独 `cc foo.c` 缺少符号 | 章节入口依赖公共实现，请使用 CMake 目标 |

退出码 `0` 表示成功，`1` 表示通信或运行错误，`2` 表示命令行或输入无效。
