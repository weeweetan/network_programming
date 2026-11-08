# 继续开发

先按 [构建说明](docs/building.md)安装依赖。原章节文件名保留用于对照书籍；新的可运行入口请在 `examples.json` 登记目标、源码、依赖、平台与模型，由 CMake 读取。公共收发逻辑放在 `common/`，不要在每个章节重复实现短写、地址解析或 fd 所有权。

新增源文件后运行 `python3 tools/catalog.py` 更新[完整索引](docs/catalog.md)。历史实验的标题与状态在该脚本中维护；只有实际纳入构建并通过相关验证的示例才能移动到可运行目标区。

## 修改时关注

- TCP 按字节流处理，检查完整写入、部分读取、EOF、EINTR 与 EAGAIN。
- 编解码使用明确的字节序与长度，不用 struct / 位域布局代替线协议。
- 明确每个 fd、FILE、事件和线程由谁释放，避免重复关闭和退出遗漏。
- 事件模型保存待发送数据，考虑背压与读半关闭后的输出排空。
- 参数、容量和数值转换先校验；尽量用真实通信场景验证行为。

## 提交前

```sh
python3 tools/catalog.py --check
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure --parallel 2
git diff --check
```

并发、缓冲区或编码修改还应运行 [ASan / UBSan 配置](docs/testing.md)。测试只使用本机临时端口与临时证书，不依赖固定公网服务或修改系统信任配置。

项目统一使用 UTF-8、LF、4 空格缩进；JSON / YAML 为 2 空格。请不要提交 IDE 部署信息、生成文件、证书私钥或构建目录。
