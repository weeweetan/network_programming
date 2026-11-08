# 示例与源文件索引

由 `python3 tools/catalog.py` 生成；新增目标请更新 `examples.json`，其他章节请更新脚本中的分类。

保留根目录全部 **66 个 C 文件**，其中 32 个入口生成 **34 个可执行目标**，7 个文件作为已构建的支持库。

“已构建”表示纳入 CMake 和回归验证；Linux 专用目标只在 Linux 构建。阅读、平台实验和草稿保留原始学习材料，未承诺可以直接编译或通过测试。

## 已维护的运行入口

| 目标 | 原文件 | 主题 | 平台 / 依赖 |
| --- | --- | --- | --- |
| `tcp-echo-server` | [TCPEchoServer.c](../TCPEchoServer.c) | 通用 TCP 顺序回显 | Linux / macOS · POSIX |
| `tcp-echo-server4` | [TCPEchoServer4.c](../TCPEchoServer4.c) | IPv4 TCP 顺序回显 | Linux / macOS · POSIX |
| `tcp-echo-client` | [TCPEchoClient.c](../TCPEchoClient.c) | 通用 TCP 单次回显客户端 | Linux / macOS · POSIX |
| `tcp-echo-client4` | [TCPEchoClient4.c](../TCPEchoClient4.c) | IPv4 TCP 单次回显客户端 | Linux / macOS · POSIX |
| `echo-server` | [echo_server.c](../echo_server.c) | 章节 TCP 回显入口 | Linux / macOS · POSIX |
| `echo-client` | [echo_client.c](../echo_client.c) | 交互式 TCP 回显 | Linux / macOS · POSIX |
| `echo-client2` | [echo_client2.c](../echo_client2.c) | 分段接收的交互式入口 | Linux / macOS · POSIX |
| `stdio-echo-server` | [echo_stdserver.c](../echo_stdserver.c) | 独立 FILE 流回显 | Linux / macOS · POSIX |
| `stdio-echo-client` | [echo_stdclient.c](../echo_stdclient.c) | FILE 示例客户端入口 | Linux / macOS · POSIX |
| `hello-server` | [hello_server.c](../hello_server.c) | 单连接问候服务 | Linux / macOS · POSIX |
| `hello-client` | [hello_client.c](../hello_client.c) | 完整读取问候响应 | Linux / macOS · POSIX |
| `tcp-server` | [tcp_server.c](../tcp_server.c) | 问候服务章节别名 | Linux / macOS · POSIX |
| `tcp-client` | [tcp_client.c](../tcp_client.c) | 问候客户端章节别名 | Linux / macOS · POSIX |
| `udp-echo-server` | [UDPEchoServer.c](../UDPEchoServer.c) | 通用 UDP 回显 | Linux / macOS · POSIX |
| `uecho-server` | [uecho_server.c](../uecho_server.c) | 章节 UDP 回显入口 | Linux / macOS · POSIX |
| `udp-echo-client` | [UDPEchoClient.c](../UDPEchoClient.c) | 已连接 UDP 回显客户端 | Linux / macOS · POSIX |
| `udp-timeout-client` | [UDPEchoClient-Timeout.c](../UDPEchoClient-Timeout.c) | 有界重试 UDP 客户端 | Linux / macOS · POSIX |
| `uecho-client` | [uecho_client.c](../uecho_client.c) | 交互式 UDP 回显 | Linux / macOS · POSIX |
| `select-echo-server` | [echo_selectserv.c](../echo_selectserv.c) | select 并发回显 | Linux / macOS · POSIX |
| `select-rot13-server` | [select_based_server.c](../select_based_server.c) | select ROT13 字节流 | Linux / macOS · POSIX |
| `chat-server` | [chat_server.c](../chat_server.c) | 按行广播的 pthread 聊天服务 | Linux / macOS · POSIX |
| `chat-client` | [chat_clnt.c](../chat_clnt.c) | 同时处理标准输入与服务端 EOF | Linux / macOS · POSIX |
| `op-server` | [op_server.c](../op_server.c) | 网络字节序算术协议服务 | Linux / macOS · POSIX |
| `op-client` | [op_client.c](../op_client.c) | 校验输入的算术客户端 | Linux / macOS · POSIX |
| `epoll-echo-server` | [echo_epollserv.c](../echo_epollserv.c) | epoll LT 回显 | Linux · POSIX |
| `epoll-lt-server` | [echo_EPLTserv.c](../echo_EPLTserv.c) | epoll LT 章节别名 | Linux · POSIX |
| `epoll-et-server` | [echo_EPETserv.c](../echo_EPETserv.c) | epoll ET + ONESHOT 回显 | Linux · POSIX |
| `libevent-rot13-server` | [libevent_based_server.c](../libevent_based_server.c) | libevent 读写事件 | Linux / macOS · libevent |
| `libevent-buffered-rot13-server` | [libevent_based_server_simply.c](../libevent_based_server_simply.c) | libevent bufferevent | Linux / macOS · libevent |
| `vote-client-bin` | [VoteClientTCP.c](../VoteClientTCP.c) | 投票客户端（bin） | Linux / macOS · POSIX |
| `vote-server-bin` | [VoteServerTCP.c](../VoteServerTCP.c) | 投票服务端（bin） | Linux / macOS · POSIX |
| `vote-client-text` | [VoteClientTCP.c](../VoteClientTCP.c) | 投票客户端（text） | Linux / macOS · POSIX |
| `vote-server-text` | [VoteServerTCP.c](../VoteServerTCP.c) | 投票服务端（text） | Linux / macOS · POSIX |
| `tls-client` | [tls_client.c](../tls_client.c) | 验证证书与名称的 TLS 客户端 | Linux / macOS · OpenSSL |

## 支持库

| 源文件 | 职责 |
| --- | --- |
| [DelimFramer.c](../DelimFramer.c) | 换行分帧；超长帧排空和下一帧对齐 |
| [DieWithMessage.c](../DieWithMessage.c) | 通用错误输出；只在调用者决定退出时使用 |
| [LengthFramer.c](../LengthFramer.c) | 两字节长度前缀；超长帧排空和截断检查 |
| [TCPClientUtility.c](../TCPClientUtility.c) | 客户端连接适配层 |
| [TCPServerUtility.c](../TCPServerUtility.c) | 服务端、地址比较和地址打印适配层 |
| [VoteEncodingBin.c](../VoteEncodingBin.c) | 4/12 字节投票消息，64 位网络字节序计数 |
| [VoteEncodingText.c](../VoteEncodingText.c) | 有界、不修改输入的文本投票编解码 |

## 公共实现

章节文件保留原名称作为入口，公共实现集中处理容易出错的收发和资源管理。对比 I/O 模型时从下表进入实际实现。

| 源文件 | 职责 |
| --- | --- |
| [common/net.c](../common/net.c) | 参数、地址解析、连接、完整读写、超时和描述符所有权 |
| [common/echo.c](../common/echo.c) | 顺序 TCP、独立 FILE 流和 UDP 回显 |
| [common/stream.c](../common/stream.c) | 有界输出队列、非阻塞收发、背压和半关闭 |
| [common/select_server.c](../common/select_server.c) | select 的读写事件和 FD_SETSIZE 检查 |
| [common/epoll_server.c](../common/epoll_server.c) | Linux LT / ET + ONESHOT 与事件重置 |
| [common/libevent_server.c](../common/libevent_server.c) | 原生事件 / bufferevent 两种模型和水位控制 |
| [common/operator.c](../common/operator.c) | int32 网络字节序和算术溢出检查 |

## 历史阅读材料和实验

| 源文件 | 状态 | 说明 |
| --- | --- | --- |
| [BruteForeCoding.c](../BruteForeCoding.c) | 阅读 | 手动字节编码练习；未纳入自动构建 |
| [GetAddrInfo.c](../GetAddrInfo.c) | 阅读 | 地址解析练习；实际结果依赖 DNS 和主机网络 |
| [SigAction.c](../SigAction.c) | 阅读 | 信号练习；包含长时间等待，未纳入自动测试 |
| [UDPEchoServer-SIGIO.c](../UDPEchoServer-SIGIO.c) | 平台实验 | SIGIO/FASYNC 异步 I/O；信号处理器中的 I/O 需单独审查 |
| [endian_conv.c](../endian_conv.c) | 阅读 | 历史字节序示例；注意 unsigned long 与 uint32_t 的宽度区别 |
| [get_buf.c](../get_buf.c) | 阅读 | 系统 socket 缓冲区查询；结果由操作系统决定 |
| [gethostbyaddr.c](../gethostbyaddr.c) | 阅读 | 旧式反向 DNS API；新示例优先使用 getnameinfo |
| [gethostbyname.c](../gethostbyname.c) | 阅读 | 旧式 IPv4 DNS API；新示例优先使用 getaddrinfo |
| [http_response.c](../http_response.c) | 草稿 | 仅文件头，尚无 HTTP 响应实现 |
| [inet_addr.c](../inet_addr.c) | 阅读 | 旧式 IPv4 地址转换；注意 INADDR_NONE 的歧义 |
| [inet_aton.c](../inet_aton.c) | 阅读 | IPv4 地址转换；可移植代码优先使用 inet_pton |
| [inet_ntoa.c](../inet_ntoa.c) | 阅读 | 静态字符串缓冲区示例；可移植代码优先使用 inet_ntop |
| [md5sig_client.c](../md5sig_client.c) | 平台实验 | Linux TCP_MD5SIG；含固定配置，需独立实验环境 |
| [md5sig_server.c](../md5sig_server.c) | 平台实验 | Linux TCP_MD5SIG；含固定配置，需独立实验环境 |
| [mutex.c](../mutex.c) | 阅读 | 互斥锁计数练习；长循环，未纳入自动测试 |
| [news_receiver.c](../news_receiver.c) | 平台实验 | 组播接收；依赖路由、网卡和组播成员配置 |
| [news_receiver_brd.c](../news_receiver_brd.c) | 平台实验 | 广播接收；依赖网段和接口配置 |
| [news_sender.c](../news_sender.c) | 平台实验 | 组播发送；依赖 news.txt、TTL 和组播路由 |
| [news_sender_brd.c](../news_sender_brd.c) | 平台实验 | 广播发送；依赖 news.txt 和广播接口 |
| [opt_client.c](../opt_client.c) | 平台实验 | 自定义 IP 选项；需核对平台、参数和网络权限 |
| [opt_server.c](../opt_server.c) | 平台实验 | 自定义 IP 选项；需核对平台、参数和网络权限 |
| [rtcp_bench.c](../rtcp_bench.c) | 草稿 | 仅文件头，尚无 RTCP 压测实现 |
| [rtcp_client.c](../rtcp_client.c) | 草稿 | 仅文件头，尚无 RTCP 客户端实现 |
| [select.c](../select.c) | 阅读 | 标准输入 select 练习；仍含 w32api 头，不能直接在 POSIX 构建 |
| [semaphore.c](../semaphore.c) | 平台实验 | 未命名信号量练习；macOS 的 sem_init 不支持此用法 |
| [sock_type.c](../sock_type.c) | 阅读 | socket 类型查询；未纳入自动构建 |
| [udp_client.c](../udp_client.c) | 平台实验 | 历史自定义 UDP 报文；C 位域布局不能作为可移植线协议 |

运行方式见 [构建与命令](building.md)，消息格式见 [协议说明](protocols.md)，覆盖范围见 [测试说明](testing.md)。
