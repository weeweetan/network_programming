#!/usr/bin/env python3
"""Generate the complete chapter/source index and check repository documentation."""
import argparse
import json
import re
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / 'docs/catalog.md'
SUPPORT = {
    'DieWithMessage.c': '通用错误输出；只在调用者决定退出时使用',
    'TCPClientUtility.c': '客户端连接适配层',
    'TCPServerUtility.c': '服务端、地址比较和地址打印适配层',
    'LengthFramer.c': '两字节长度前缀；超长帧排空和截断检查',
    'DelimFramer.c': '换行分帧；超长帧排空和下一帧对齐',
    'VoteEncodingBin.c': '4/12 字节投票消息，64 位网络字节序计数',
    'VoteEncodingText.c': '有界、不修改输入的文本投票编解码',
}
READING = {
    'BruteForeCoding.c': ('阅读', '手动字节编码练习；未纳入自动构建'),
    'GetAddrInfo.c': ('阅读', '地址解析练习；实际结果依赖 DNS 和主机网络'),
    'SigAction.c': ('阅读', '信号练习；包含长时间等待，未纳入自动测试'),
    'UDPEchoServer-SIGIO.c': ('平台实验', 'SIGIO/FASYNC 异步 I/O；信号处理器中的 I/O 需单独审查'),
    'endian_conv.c': ('阅读', '历史字节序示例；注意 unsigned long 与 uint32_t 的宽度区别'),
    'get_buf.c': ('阅读', '系统 socket 缓冲区查询；结果由操作系统决定'),
    'gethostbyaddr.c': ('阅读', '旧式反向 DNS API；新示例优先使用 getnameinfo'),
    'gethostbyname.c': ('阅读', '旧式 IPv4 DNS API；新示例优先使用 getaddrinfo'),
    'http_response.c': ('草稿', '仅文件头，尚无 HTTP 响应实现'),
    'inet_addr.c': ('阅读', '旧式 IPv4 地址转换；注意 INADDR_NONE 的歧义'),
    'inet_aton.c': ('阅读', 'IPv4 地址转换；可移植代码优先使用 inet_pton'),
    'inet_ntoa.c': ('阅读', '静态字符串缓冲区示例；可移植代码优先使用 inet_ntop'),
    'md5sig_client.c': ('平台实验', 'Linux TCP_MD5SIG；含固定配置，需独立实验环境'),
    'md5sig_server.c': ('平台实验', 'Linux TCP_MD5SIG；含固定配置，需独立实验环境'),
    'mutex.c': ('阅读', '互斥锁计数练习；长循环，未纳入自动测试'),
    'news_receiver.c': ('平台实验', '组播接收；依赖路由、网卡和组播成员配置'),
    'news_receiver_brd.c': ('平台实验', '广播接收；依赖网段和接口配置'),
    'news_sender.c': ('平台实验', '组播发送；依赖 news.txt、TTL 和组播路由'),
    'news_sender_brd.c': ('平台实验', '广播发送；依赖 news.txt 和广播接口'),
    'opt_client.c': ('平台实验', '自定义 IP 选项；需核对平台、参数和网络权限'),
    'opt_server.c': ('平台实验', '自定义 IP 选项；需核对平台、参数和网络权限'),
    'rtcp_bench.c': ('草稿', '仅文件头，尚无 RTCP 压测实现'),
    'rtcp_client.c': ('草稿', '仅文件头，尚无 RTCP 客户端实现'),
    'select.c': ('阅读', '标准输入 select 练习；仍含 w32api 头，不能直接在 POSIX 构建'),
    'semaphore.c': ('平台实验', '未命名信号量练习；macOS 的 sem_init 不支持此用法'),
    'sock_type.c': ('阅读', 'socket 类型查询；未纳入自动构建'),
    'udp_client.c': ('平台实验', '历史自定义 UDP 报文；C 位域布局不能作为可移植线协议'),
}
COMMON = {
    'net.c': '参数、地址解析、连接、完整读写、超时和描述符所有权',
    'echo.c': '顺序 TCP、独立 FILE 流和 UDP 回显',
    'stream.c': '有界输出队列、非阻塞收发、背压和半关闭',
    'select_server.c': 'select 的读写事件和 FD_SETSIZE 检查',
    'epoll_server.c': 'Linux LT / ET + ONESHOT 与事件重置',
    'libevent_server.c': '原生事件 / bufferevent 两种模型和水位控制',
    'operator.c': 'int32 网络字节序和算术溢出检查',
}


def manifest():
    entries = json.loads((ROOT / 'examples.json').read_text(encoding='utf-8'))
    names = set()
    by_source = {}
    for entry in entries:
        name = entry['name']
        assert re.fullmatch(r'[a-z0-9]+(?:-[a-z0-9]+)*', name), name
        assert name not in names, f'duplicate target: {name}'
        names.add(name)
        assert entry['feature'] in {'core', 'libevent', 'tls'}, name
        assert entry['platform'] in {'posix', 'linux'}, name
        assert entry['libraries'], name
        assert (ROOT / entry['source']).is_file(), entry['source']
        by_source.setdefault(entry['source'], []).append(entry)
    expected = set(by_source) | set(SUPPORT) | set(READING)
    actual = {path.name for path in ROOT.glob('*.c')}
    assert actual == expected, f'unindexed: {actual - expected}; missing: {expected - actual}'
    assert not (set(by_source) & set(SUPPORT) or set(by_source) & set(READING) or set(SUPPORT) & set(READING))
    assert {path.name for path in (ROOT / 'common').glob('*.c')} == set(COMMON)
    return entries, by_source, actual


def render(entries, by_source, sources):
    lines = [
        '# 示例与源文件索引', '',
        '由 `python3 tools/catalog.py` 生成；新增目标请更新 `examples.json`，其他章节请更新脚本中的分类。', '',
        f'保留根目录全部 **{len(sources)} 个 C 文件**，其中 {len(by_source)} 个入口生成 '
        f'**{len(entries)} 个可执行目标**，{len(SUPPORT)} 个文件作为已构建的支持库。', '',
        '“已构建”表示纳入 CMake 和回归验证；Linux 专用目标只在 Linux 构建。'
        '阅读、平台实验和草稿保留原始学习材料，未承诺可以直接编译或通过测试。', '',
        '## 已维护的运行入口', '',
        '| 目标 | 原文件 | 主题 | 平台 / 依赖 |',
        '| --- | --- | --- | --- |',
    ]
    for entry in entries:
        platform = 'Linux' if entry['platform'] == 'linux' else 'Linux / macOS'
        dependency = {'core': 'POSIX', 'tls': 'OpenSSL', 'libevent': 'libevent'}[entry['feature']]
        lines.append(f"| `{entry['name']}` | [{entry['source']}](../{entry['source']}) | "
                     f"{entry['title']} | {platform} · {dependency} |")
    lines += ['', '## 支持库', '', '| 源文件 | 职责 |', '| --- | --- |']
    for source, title in sorted(SUPPORT.items()):
        lines.append(f'| [{source}](../{source}) | {title} |')
    lines += ['', '## 公共实现', '',
              '章节文件保留原名称作为入口，公共实现集中处理容易出错的收发和资源管理。'
              '对比 I/O 模型时从下表进入实际实现。', '',
              '| 源文件 | 职责 |', '| --- | --- |']
    for source, title in COMMON.items():
        lines.append(f'| [common/{source}](../common/{source}) | {title} |')
    lines += ['', '## 历史阅读材料和实验', '', '| 源文件 | 状态 | 说明 |', '| --- | --- | --- |']
    for source, (status, title) in sorted(READING.items()):
        lines.append(f'| [{source}](../{source}) | {status} | {title} |')
    lines += ['', '运行方式见 [构建与命令](building.md)，消息格式见 [协议说明](protocols.md)，'
              '覆盖范围见 [测试说明](testing.md)。', '']
    return '\n'.join(lines)


def check_links():
    documents = [ROOT / 'README.md', ROOT / 'CONTRIBUTING.md', *(ROOT / 'docs').glob('*.md')]
    for document in documents:
        text = document.read_text(encoding='utf-8')
        for link in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)', text):
            parsed = urlsplit(link.strip('<>'))
            if parsed.scheme or parsed.netloc or not parsed.path:
                continue
            target = (document.parent / unquote(parsed.path)).resolve()
            assert target.is_relative_to(ROOT) and target.is_file(), f'{document.name}: broken link {link}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='validate without changing files')
    args = parser.parse_args()
    entries, by_source, sources = manifest()
    expected = render(entries, by_source, sources)
    if args.check:
        assert CATALOG.read_text(encoding='utf-8') == expected, 'run python3 tools/catalog.py to refresh docs/catalog.md'
    else:
        CATALOG.parent.mkdir(exist_ok=True)
        CATALOG.write_text(expected, encoding='utf-8')
    maintained = [*(ROOT / source for source in by_source), *(ROOT / source for source in SUPPORT),
                  *(ROOT / 'common').glob('*.[ch]'), *(ROOT / 'tests').glob('*.[ch]')]
    for path in maintained:
        assert path.read_bytes().endswith(b'\n'), f'{path.name}: missing final newline'
    check_links()
    print(f'Catalog OK: {len(entries)} targets, all {len(sources)} original C sources indexed; local documentation links valid.')


if __name__ == '__main__':
    main()
