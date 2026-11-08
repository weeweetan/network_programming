"""Real loopback-only integration checks; no Internet services or fixed ports."""
import argparse
import contextlib
import os
import selectors
import socket
import ssl
import struct
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

ARGS = None


def executable(name):
    return str(ARGS.bin_dir / name)


def clean_diagnostics(data):
    for marker in [b'AddressSanitizer', b'UndefinedBehaviorSanitizer', b'runtime error:', b'LeakSanitizer']:
        assert marker not in data, data.decode(errors='replace')


def run(name, *arguments, input=b'', code=0, timeout=10):
    result = subprocess.run([executable(name), *map(str, arguments)], input=input,
                            capture_output=True, timeout=timeout)
    clean_diagnostics(result.stderr)
    assert result.returncode == code, (name, result.returncode, result.stdout, result.stderr)
    return result


class Server:
    def __init__(self, name, limit, host='127.0.0.1'):
        self.name, self.limit, self.host = name, limit, host

    def __enter__(self):
        self.errors = tempfile.TemporaryFile()
        self.process = subprocess.Popen([executable(self.name), '0', self.host, str(self.limit)],
                                        stdout=subprocess.PIPE, stderr=self.errors)
        try:
            with selectors.DefaultSelector() as selector:
                selector.register(self.process.stdout, selectors.EVENT_READ)
                assert selector.select(10), f'{self.name}: no readiness message'
                line = self.process.stdout.readline()
                assert line.startswith(b'LISTEN '), (self.name, line)
                self.port = int(line.split()[-1])
            return self
        except BaseException:
            self.stop()
            self.errors.seek(0)
            print(self.errors.read().decode(errors='replace'))
            self.process.stdout.close()
            self.errors.close()
            raise

    def connect(self):
        connection = socket.create_connection((self.host, self.port), timeout=5)
        connection.settimeout(5)
        return connection

    def stop(self):
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=7)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()

    def __exit__(self, kind, value, traceback):
        try:
            if kind is None:
                try:
                    code = self.process.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    self.stop()
                    raise AssertionError(f'{self.name}: did not finish after LIMIT connections/datagrams')
                self.errors.seek(0)
                errors = self.errors.read()
                clean_diagnostics(errors)
                assert code == 0, (self.name, code, errors.decode(errors='replace'))
            else:
                self.stop()
        finally:
            self.process.stdout.close()
            self.errors.close()


def receive_all(connection):
    parts = []
    while True:
        chunk = connection.recv(997)
        if not chunk:
            return b''.join(parts)
        parts.append(chunk)


def receive_exact(connection, length):
    result = bytearray()
    while len(result) < length:
        chunk = connection.recv(length - len(result))
        assert chunk, f'EOF after {len(result)} of {length} bytes'
        result.extend(chunk)
    return bytes(result)


def transform(payload):
    if not ARGS.rot13:
        return payload
    source = b'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
    target = b'nopqrstuvwxyzabcdefghijklmNOPQRSTUVWXYZABCDEFGHIJKLM'
    return payload.translate(bytes.maketrans(source, target))


def echo_tests():
    payload = bytes(range(256)) * 2048 + b'tail without newline'
    with Server(ARGS.server, 1) as server, server.connect() as connection:
        connection.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 4096)
        failures = []

        def send():
            try:
                offset = 0
                for size in [1, 3, 17, 1024]:
                    connection.sendall(payload[offset:offset + size])
                    offset += size
                connection.sendall(payload[offset:])
                connection.shutdown(socket.SHUT_WR)
            except BaseException as error:
                failures.append(error)

        writer = threading.Thread(target=send, daemon=True)
        writer.start()
        time.sleep(0.05)  # Let the output queue experience backpressure.
        result = receive_all(connection)
        writer.join(timeout=5)
        assert not writer.is_alive() and not failures, failures
        assert result == transform(payload), (ARGS.server, len(result), len(payload))

    with Server(ARGS.server, 9) as server:
        for index in range(8):
            message = b'repeat\0' + str(index).encode()
            with server.connect() as connection:
                connection.sendall(message)
                connection.shutdown(socket.SHUT_WR)
                assert receive_all(connection) == transform(message)
        with server.connect() as reset:
            # Read a complete chunk first, so this connection was actually accepted.
            reset.sendall(b'abandon\n')
            assert receive_exact(reset, 8) == transform(b'abandon\n')
            reset.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))

    if ARGS.multiplex:
        with Server(ARGS.server, 4) as server, contextlib.ExitStack() as sockets:
            idle = sockets.enter_context(server.connect())
            active = sockets.enter_context(server.connect())
            active.sendall(b'live\0peer')
            assert receive_exact(active, 9) == transform(b'live\0peer')
            with server.connect() as reset:
                reset.sendall(b'reset')
                assert receive_exact(reset, 5) == transform(b'reset')
                reset.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
            with server.connect() as final:
                final.sendall(b'after reset')
                final.shutdown(socket.SHUT_WR)
                assert receive_all(final) == transform(b'after reset')
            active.shutdown(socket.SHUT_WR)
            assert receive_all(active) == b''
            idle.shutdown(socket.SHUT_WR)
            assert receive_all(idle) == b''


def client_tests():
    for client, server, message in [('tcp-echo-client', 'tcp-echo-server', 'Hello 世界'),
                                    ('tcp-echo-client4', 'tcp-echo-server4', ''),
                                    ('tcp-echo-client', 'stdio-echo-server', 'no newline')]:
        with Server(server, 1) as instance:
            result = run(client, 'localhost', message, instance.port)
            assert result.stdout == b'Received: ' + message.encode() + b'\n'
    data = b'alpha\n' + b'x' * 2050 + b'\ntail'
    for client, server in [('echo-client', 'echo-server'), ('echo-client2', 'echo-server'),
                           ('stdio-echo-client', 'stdio-echo-server')]:
        with Server(server, 1) as instance:
            assert run(client, '127.0.0.1', instance.port, input=data).stdout == data
        with Server(server, 1) as instance:
            assert run(client, '127.0.0.1', instance.port, input=b'').stdout == b''
    for client, server in [('hello-client', 'hello-server'), ('tcp-client', 'tcp-server')]:
        with Server(server, 1) as instance:
            assert run(client, '127.0.0.1', instance.port).stdout == b'Hello World!\n'
    try:
        probe = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
        with probe:
            probe.bind(('::1', 0))
    except OSError:
        print('IPv6 loopback unavailable; IPv4 checks still run.')
    else:
        with Server('tcp-echo-server', 1, '::1') as instance:
            assert run('tcp-echo-client', '::1', 'ipv6', instance.port).stdout == b'Received: ipv6\n'
    bad = [('echo-server', ['65536']), ('tcp-echo-server', ['-1']),
           ('tcp-echo-client', ['127.0.0.1', 'x', '70000']),
           ('vote-client-bin', ['127.0.0.1', '1']),
           ('vote-client-text', ['127.0.0.1', '1', '-1']),
           ('chat-client', ['127.0.0.1', '1', 'x' * 17]),
           ('op-client', ['127.0.0.1', '0'])]
    for name, arguments in bad:
        run(name, *arguments, code=2)


def udp_tests():
    # macOS has a smaller default UDP send limit than Linux. Do not change sysctls.
    large_payload = 8000 if sys.platform == 'darwin' else 65000
    for name in ['udp-echo-server', 'uecho-server']:
        with Server(name, 4) as server, socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client:
            client.settimeout(4)
            for message in [b'', b'x', bytes(range(256)) * 4, b'z' * large_payload]:
                client.sendto(message, ('127.0.0.1', server.port))
                response, _ = client.recvfrom(65536)
                assert response == message
    for name in ['udp-echo-client', 'udp-timeout-client']:
        with Server('udp-echo-server', 1) as server:
            assert run(name, '127.0.0.1', '', server.port).stdout == b'Received: \n'
    with Server('udp-echo-server', 1) as server:
        assert run('uecho-client', '127.0.0.1', server.port, input=b'udp\n').stdout == b'Received: udp\n\n'
    with Server('udp-echo-server', 0) as server:
        assert run('uecho-client', '127.0.0.1', server.port, input=b'').stdout == b''
        server.stop()

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as peer:
        peer.bind(('127.0.0.1', 0)); peer.settimeout(4)
        received, failures = [], []

        def retry_peer():
            try:
                first, address = peer.recvfrom(2048); received.append(first)
                second, again = peer.recvfrom(2048); received.append(second)
                assert address == again
                peer.sendto(second, address)
            except BaseException as error:
                failures.append(error)

        worker = threading.Thread(target=retry_peer, daemon=True); worker.start()
        assert run('udp-timeout-client', '127.0.0.1', 'retry', peer.getsockname()[1]).stdout == b'Received: retry\n'
        worker.join(timeout=5)
        assert not worker.is_alive() and not failures and received == [b'retry', b'retry'], failures
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as peer, \
            socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as stranger:
        peer.bind(('127.0.0.1', 0)); peer.settimeout(4)
        stranger.bind(('127.0.0.1', 0))
        failures = []

        def source_peer():
            try:
                message, address = peer.recvfrom(2048)
                stranger.sendto(b'wrong sender', address)
                time.sleep(0.05)
                peer.sendto(message, address)
            except BaseException as error:
                failures.append(error)

        worker = threading.Thread(target=source_peer, daemon=True); worker.start()
        assert run('udp-echo-client', '127.0.0.1', 'source', peer.getsockname()[1]).stdout == b'Received: source\n'
        worker.join(timeout=5)
        assert not worker.is_alive() and not failures, failures
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as silent:
        silent.bind(('127.0.0.1', 0))
        start = time.monotonic()
        run('udp-timeout-client', '127.0.0.1', 'timeout', silent.getsockname()[1], code=1, timeout=4)
        assert time.monotonic() - start < 3


def vote_request(kind, candidate, inquiry=False):
    if kind == 'bin':
        body = struct.pack('!HH', 0x5400 | (0x100 if inquiry else 0), candidate)
        return struct.pack('!H', len(body)) + body
    return f'Voting {"i" if inquiry else "v"} {candidate}\n'.encode()


def vote_response(connection, kind):
    if kind == 'bin':
        size = struct.unpack('!H', receive_exact(connection, 2))[0]
        assert size == 12
        header, candidate, count = struct.unpack('!HHQ', receive_exact(connection, size))
        assert header & 0x0200
        return candidate, bool(header & 0x0100), count
    line = bytearray()
    while not line.endswith(b'\n'):
        line.extend(receive_exact(connection, 1))
        assert len(line) < 500
    magic, operation, flag, candidate, count = line.split()
    assert magic == b'Voting' and flag == b'R'
    return int(candidate), operation == b'i', int(count)


def vote_tests():
    for kind in ['bin', 'text']:
        with Server('vote-server-' + kind, 3) as server:
            with server.connect() as connection:
                requests = vote_request(kind, 23) + vote_request(kind, 23, True) + vote_request(kind, 23)
                for byte in requests:
                    connection.sendall(bytes([byte]))
                    time.sleep(0.001)
                assert vote_response(connection, kind) == (23, False, 1)
                assert vote_response(connection, kind) == (23, True, 1)
                assert vote_response(connection, kind) == (23, False, 2)
            with server.connect() as broken:
                broken.sendall(b'\x00\x04\x00\x00\x00\x00' if kind == 'bin' else b'Voting v -1\n')
                assert broken.recv(1) == b''
            assert run('vote-client-' + kind, '127.0.0.1', server.port, 23, 'I').stdout == b'candidate=23 inquiry=1 count=2\n'


def chat_tests():
    with Server('chat-server', 3) as server, contextlib.ExitStack() as sockets:
        connections = []
        for index in range(3):
            connection = sockets.enter_context(server.connect())
            connections.append(connection)
            probe = f'joined {index}\n'.encode()
            connection.sendall(probe)
            for registered in connections:
                assert receive_exact(registered, len(probe)) == probe
        messages = [b'one\n', b'two\n', b'three\n']
        for connection, message in zip(connections, messages):
            connection.sendall(message[:1]); connection.sendall(message[1:])
        for connection in connections:
            received = receive_exact(connection, sum(map(len, messages)))
            assert sorted(received.splitlines(keepends=True)) == sorted(messages)
    with Server('chat-server', 1) as server:
        assert run('chat-client', '127.0.0.1', server.port, 'alice', input=b'hello\n').stdout == b'[alice] hello\n'
    with Server('chat-server', 1) as server:
        assert run('chat-client', '127.0.0.1', server.port, 'alice', input=b'').stdout == b''


def arithmetic_tests():
    def packet(values, operation):
        return bytes([len(values)]) + b''.join(struct.pack('!i', v) for v in values) + operation

    with Server('op-server', 6) as server:
        with server.connect() as connection:
            for byte in packet([10, -2, 7], b'+'):
                connection.sendall(bytes([byte]))
            assert struct.unpack('!i', receive_exact(connection, 4))[0] == 15
        for invalid in [b'\x00', b'\x02' + struct.pack('!i', 1), packet([2147483647, 1], b'+'), packet([1], b'/')]:
            with server.connect() as connection:
                connection.sendall(invalid); connection.shutdown(socket.SHUT_WR)
                assert connection.recv(1) == b''
        result = run('op-client', '127.0.0.1', server.port, input=b'3\n10\n-2\n7\n+\n')
        assert b'Operation result: 15\n' in result.stdout
    for input in [b'', b'0\n', b'65\n', b'1\n9999999999999999999999999\n', b'1\n1\n\n']:
        run('op-client', '127.0.0.1', 1, input=input, code=2)


class LocalTLS:
    def __init__(self, certificate, key, payload, stall=False, abrupt=False):
        self.context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        self.context.minimum_version = ssl.TLSVersion.TLSv1_2
        self.context.load_cert_chain(certificate, key)
        self.names = []
        self.context.set_servername_callback(lambda connection, name, context: self.names.append(name))
        self.payload, self.stall = payload, stall
        self.abrupt = abrupt
        self.request = b''
        self.failure = None

    def __enter__(self):
        self.listener = socket.socket()
        self.listener.bind(('127.0.0.1', 0)); self.listener.listen(1); self.listener.settimeout(8)
        self.port = self.listener.getsockname()[1]

        def serve():
            try:
                raw, _ = self.listener.accept(); raw.settimeout(8)
                with raw:
                    if self.stall:
                        while raw.recv(4096):
                            pass
                    else:
                        with self.context.wrap_socket(raw, server_side=True) as connection:
                            while b'\r\n\r\n' not in self.request:
                                chunk = connection.recv(4096)
                                if not chunk:
                                    break
                                self.request += chunk
                                assert len(self.request) < 8192
                            connection.sendall(self.payload[:23]); connection.sendall(self.payload[23:])
                            if self.abrupt:
                                os.close(connection.detach())  # Deliberately omit TLS close_notify.
                            else:
                                connection.unwrap().close()
            except BaseException as error:
                self.failure = error

        self.thread = threading.Thread(target=serve, daemon=True); self.thread.start()
        return self

    def __exit__(self, kind, value, traceback):
        self.thread.join(timeout=9)
        self.listener.close()
        assert not self.thread.is_alive(), 'TLS fixture did not terminate'


def tls_tests():
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)

        def openssl(*arguments):
            result = subprocess.run([ARGS.openssl, *map(str, arguments)], cwd=root, capture_output=True, timeout=15)
            assert result.returncode == 0, result.stderr

        (root / 'ca.cnf').write_text('''[req]
prompt=no
distinguished_name=dn
x509_extensions=ca
[dn]
CN=LocalNetworkTestCA
[ca]
basicConstraints=critical,CA:TRUE
keyUsage=critical,keyCertSign,cRLSign
subjectKeyIdentifier=hash
''')
        (root / 'leaf.ext').write_text('''basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=DNS:localhost,IP:127.0.0.1,IP:::1
authorityKeyIdentifier=keyid,issuer
''')
        openssl('req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-days', '1', '-config', 'ca.cnf', '-keyout', 'ca.key', '-out', 'ca.crt')
        openssl('req', '-new', '-newkey', 'rsa:2048', '-nodes', '-subj', '/CN=localhost', '-keyout', 'server.key', '-out', 'server.csr')
        openssl('x509', '-req', '-in', 'server.csr', '-CA', 'ca.crt', '-CAkey', 'ca.key', '-CAcreateserial', '-days', '1', '-extfile', 'leaf.ext', '-out', 'server.crt')
        body = bytes(range(256)) * 1024
        response = b'HTTP/1.1 200 OK\r\nContent-Length: ' + str(len(body)).encode() + b'\r\nConnection: close\r\n\r\n' + body
        for explicit_name in [True, False]:
            with LocalTLS(root / 'server.crt', root / 'server.key', response) as server:
                arguments = ['127.0.0.1', server.port, root / 'ca.crt']
                if explicit_name:
                    arguments.append('localhost')
                assert run('tls-client', *arguments).stdout == response
            assert server.failure is None, server.failure
            name = 'localhost' if explicit_name else '127.0.0.1'
            assert f'Host: {name}:{server.port}\r\n'.encode() in server.request
            assert server.names == (['localhost'] if explicit_name else [None])
        with LocalTLS(root / 'server.crt', root / 'server.key', response) as server:
            run('tls-client', '127.0.0.1', server.port, root / 'ca.crt', 'wrong.invalid', code=1)
        assert not server.request
        with LocalTLS(root / 'server.crt', root / 'server.key', response, abrupt=True) as server:
            run('tls-client', '127.0.0.1', server.port, root / 'ca.crt', code=1)
        assert server.failure is None, server.failure
        with LocalTLS(root / 'server.crt', root / 'server.key', response) as server:
            run('tls-client', '127.0.0.1', server.port, code=1)
        assert not server.request
        with LocalTLS(root / 'server.crt', root / 'server.key', b'', stall=True) as server:
            start = time.monotonic()
            run('tls-client', '127.0.0.1', server.port, root / 'ca.crt', 'localhost', code=1, timeout=8)
            assert time.monotonic() - start < 7
        run('tls-client', '127.0.0.1', 443, root / 'ca.crt', 'bad\r\nHost: injected', code=2)


def main():
    global ARGS
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin-dir', type=Path, required=True)
    parser.add_argument('--case', required=True)
    parser.add_argument('--server')
    parser.add_argument('--rot13', action='store_true')
    parser.add_argument('--multiplex', action='store_true')
    parser.add_argument('--openssl', default='openssl')
    ARGS = parser.parse_args()
    ARGS.bin_dir = ARGS.bin_dir.resolve()
    cases = {'echo': echo_tests, 'clients': client_tests, 'udp': udp_tests, 'votes': vote_tests,
             'chat': chat_tests, 'arithmetic': arithmetic_tests, 'tls': tls_tests}
    cases[ARGS.case]()
    print(f'{ARGS.case}: loopback integration checks passed')


if __name__ == '__main__':
    main()
