# 测试与支持范围

## 运行检查

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure --parallel 2
python3 tools/catalog.py --check
```

内存与未定义行为检查使用独立构建目录：

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build-asan --parallel
# Linux 可以启用 detect_leaks=1；macOS 的 LeakSanitizer 不受支持
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build-asan --output-on-failure --parallel 2
```

CI 的 Linux GCC 普通构建、Linux Clang ASan/UBSan 和 macOS Clang ASan/UBSan 都开启 TLS / libevent；Linux Clang 另外开启泄漏检查。Linux 完整配置有 24 组 CTest，macOS 完整配置有 21 组；关闭两个可选依赖后分别为 21 / 18 组。测试数包含文档检查；每个集成测试包含多个连接与故障场景。

## 覆盖场景

| 检查 | 核心场景 |
| --- | --- |
| 两种 Framer | 空帧、NUL 字节、60000 字节超长帧排空、下一帧对齐、部分 EOF、刷新失败 |
| 两种投票 Codec | 64 位边界计数、非对齐内存、精确容量、坏长度 / 标记 / 数字、输入不变 |
| socket 公共层 | 小发送缓冲区下的 1 MiB 完整写入、截断检测、SIGPIPE、FILE 描述符所有权 |
| TCP 与事件模型 | 512 KiB 二进制分段消息、延迟读取、半关闭、反复连接、RST 后继续服务 |
| 并发模型 | 空闲连接与活跃连接并存、连接错误隔离、已排队输出不丢失 |
| 客户端 | 长输入行、无换行尾部、空 stdin、UTF-8、无效端口、可用时 IPv6 回环 |
| UDP | 空数据报、二进制 / 大数据报、首次丢包后重试、来源过滤、超时结束 |
| 投票通信 | 一字节分片、同连接多个请求、查询计数不变、坏请求后新连接正常 |
| 聊天室 | 多连接注册、片段合并、广播行不交错、客户端 EOF 退出 |
| 算术通信 | 分段请求、无效数量 / 操作符、截断、int32 溢出、标准输入错误 |
| TLS | 临时 CA / 证书、DNS + SNI、IP SAN、不可信根、名称错误、超时、256 KiB 以上二进制响应 |
| 文档 | 全部 66 个原始 C 文件有索引、目标唯一、文档链接有效、生成索引同步 |

集成测试只连接 `127.0.0.1` / `::1`，使用系统分配的临时端口，不请求公网 TLS / DNS 服务，不要求 root 权限，不修改系统配置。UDP 大包在 Linux 使用 65000 字节，macOS 使用 8000 字节以适应系统默认限制。IPv6 不可用时只略过 IPv6 场景。

TLS 测试需要 Python ssl 模块和 openssl 命令，在临时目录生成有效期 1 天的测试 CA 和证书，用完自动删除。不会写入系统信任库。单个 CTest 超时为 60 秒；服务端通过有限连接 / 数据报计数退出，失败时测试脚本终止并回收进程。

## 资源和边界

阻塞 TCP 示例使用约 5 秒的 socket 收发超时，适用于本机学习交互；DNS 解析时间由系统 resolver 决定。并发回显最多 128 个活跃连接，使用有界队列；没有实现应用层空闲踢出或生产服务的负载调度。达到连接容量后暂停接受，内核 backlog 仍由系统管理。

聊天服务使用最多 128 个可 join 的工作线程，关闭时通知并回收；慢读接收方会被断开。投票服务为顺序处理、内存计数；不包含持久化、鉴权和多进程一致性。TLS 客户端只演示传输与身份验证，不解析 HTTP 状态或 chunked 编码。

根目录仍保留 27 个历史阅读 / 平台实验 / 草稿文件，具体状态见[源码索引](catalog.md)。它们不属于自动构建与通信测试覆盖范围。特别是组播、广播、IP 选项、TCP MD5 和 SIGIO 需要独立平台与网络实验，不能用基础回环测试的结果替代验证。
