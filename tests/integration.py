#!/usr/bin/env python3
"""Black-box HTTP and resource regressions; standard library only, macOS/Linux."""
import concurrent.futures
import os
from pathlib import Path
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import time
import unittest

PROJECT = Path(__file__).resolve().parents[1]
BINARY = Path(os.environ.get('WEBSERV_BINARY', str(PROJECT / 'webserv'))).resolve()
TIMEOUT = float(os.environ.get('WEBSERV_TEST_TIMEOUT', '3'))


def request(path='/', method='GET', body=b'', headers=b'', host='local.test', close=True):
    return (f'{method} {path} HTTP/1.1\r\nHost: {host}\r\n'.encode()
            + (b'Connection: close\r\n' if close else b'') + headers
            + (f'Content-Length: {len(body)}\r\n'.encode() if method == 'POST' else b'')
            + b'\r\n' + body)


class Wire:
    """Preserve surplus bytes so pipelining tests verify response boundaries."""
    def __init__(self, sock):
        self.sock, self.buffer = sock, b''

    def read(self):
        deadline = time.monotonic() + TIMEOUT
        def receive():
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError('response deadline exceeded')
            self.sock.settimeout(remaining)
            data = self.sock.recv(65536)
            if not data:
                raise AssertionError('connection closed before complete response')
            self.buffer += data
        while b'\r\n\r\n' not in self.buffer:
            receive()
            if len(self.buffer) > 65536:
                raise AssertionError('response headers exceed 64 KiB')
        head, self.buffer = self.buffer.split(b'\r\n\r\n', 1)
        lines = head.split(b'\r\n')
        status = int(lines[0].split()[1])
        headers = {}
        for line in lines[1:]:
            key, value = line.split(b':', 1)
            key = key.lower()
            if key in headers:
                raise AssertionError(f'duplicate response header: {key!r}')
            headers[key] = value.strip()
        if status == 204:
            length = int(headers.get(b'content-length', b'0'))
            if length:
                raise AssertionError('204 response must have no body')
        else:
            if b'content-length' not in headers:
                raise AssertionError('expected Content-Length response framing')
            length = int(headers[b'content-length'])
        if not 0 <= length <= 16 * 1024 * 1024:
            raise AssertionError(f'invalid response length: {length}')
        while len(self.buffer) < length:
            receive()
        body, self.buffer = self.buffer[:length], self.buffer[length:]
        return status, headers, body


class Integration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix='webserv-integration-')
        cls.addClassCleanup(cls.tmp.cleanup)
        cls.base = Path(cls.tmp.name)
        cls.root = cls.base / 'www'
        cls.root.mkdir()
        cls.uploads = cls.root / 'upload'
        cls.uploads.mkdir()
        (cls.root / 'index.html').write_bytes(b'primary-index\n')
        cls.large = bytes(range(256)) * 16384
        (cls.root / 'large.bin').write_bytes(cls.large)
        (cls.base / 'secret.txt').write_bytes(b'OUTSIDE-ROOT-SECRET')
        (cls.root / 'escape').symlink_to(cls.base / 'secret.txt')
        cgi = cls.root / 'cgi'
        cgi.mkdir()
        (cgi / 'hello.py').write_text('print("Content-Type: text/plain\\r\\n\\r\\nCGI-EXECUTED", end="")\n')
        second = cls.base / 'second'
        second.mkdir()
        (second / 'index.html').write_bytes(b'second-index\n')
        with socket.socket() as probe:
            probe.bind(('127.0.0.1', 0))
            cls.port = probe.getsockname()[1]
        config = cls.base / 'server.conf'
        config.write_text(f'''server {{
 listen 127.0.0.1:{cls.port};
 server_name local.test;
 root {cls.root};
 client_max_body_size 1024;
 location / {{ root {cls.root}; allow_methods GET; index index.html; }}
 location /upload {{ root {cls.uploads}; allow_methods GET POST DELETE; upload_store {cls.uploads}; autoindex on; }}
 location /old {{ return 301 /; }}
 location /cgi {{ root {cgi}; allow_methods GET POST; cgi_extension .py; cgi_path {sys.executable}; }}
}}
server {{
 listen 127.0.0.1:{cls.port};
 server_name second.test;
 root {second};
 client_max_body_size 8;
 location / {{ root {second}; allow_methods GET; index index.html; }}
 location /upload {{ root {cls.uploads}; allow_methods POST; upload_store {cls.uploads}; }}
}}
''')
        cls.log = tempfile.TemporaryFile()
        cls.addClassCleanup(cls.log.close)
        cls.process = subprocess.Popen([str(BINARY), str(config)], cwd=PROJECT,
                                       stdout=cls.log, stderr=cls.log)
        cls.addClassCleanup(cls.stop_server)
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if cls.process.poll() is not None:
                cls.log.seek(0)
                raise RuntimeError(cls.log.read().decode(errors='replace'))
            try:
                with cls.connect():
                    return
            except OSError:
                time.sleep(.05)
        raise RuntimeError('server did not start within 5 seconds')

    @classmethod
    def stop_server(cls):
        if cls.process.poll() is None:
            cls.process.terminate()
            try:
                cls.process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                cls.process.kill()
                cls.process.wait(timeout=3)
        cls.log.seek(0)
        output = cls.log.read().decode(errors='replace')
        if 'ERROR: AddressSanitizer' in output or 'runtime error:' in output:
            raise AssertionError('sanitizer reported an error:\n' + output[-12000:])

    @classmethod
    def connect(cls):
        return socket.create_connection(('127.0.0.1', cls.port), TIMEOUT)

    def exchange(self, data):
        with self.connect() as sock:
            sock.sendall(data)
            return Wire(sock).read()

    def tearDown(self):
        self.assertIsNone(self.process.poll(), 'server crashed during test')

    def test_static_binary_and_missing(self):
        self.assertEqual(self.exchange(request('/large.bin'))[2], self.large)
        self.assertEqual(self.exchange(request('/missing'))[0], 404)

    def test_virtual_hosts_and_fallback(self):
        for host, expected in [('local.test', b'primary-index\n'),
                               ('second.test', b'second-index\n'),
                               ('unknown.test', b'primary-index\n'),
                               (f'second.test:{self.port}', b'second-index\n')]:
            with self.subTest(host=host):
                self.assertEqual(self.exchange(request(host=host))[2], expected)

    def test_host_case_insensitive(self):
        self.assertEqual(self.exchange(request(host='SECOND.TEST'))[2], b'second-index\n')

    def test_configured_cgi_executes(self):
        status, _, body = self.exchange(request('/cgi/hello.py'))
        self.assertEqual(status, 200)
        self.assertEqual(body, b'CGI-EXECUTED', 'CGI must execute instead of serving script source')

    def test_http10_default_close(self):
        with self.connect() as sock:
            sock.sendall(b'GET / HTTP/1.0\r\nHost: local.test\r\n\r\n')
            self.assertEqual(Wire(sock).read()[0], 200)
            self.assertEqual(sock.recv(1), b'')

    def test_redirect_and_method_permissions(self):
        status, headers, body = self.exchange(request('/old'))
        self.assertEqual((status, headers.get(b'location'), body), (301, b'/', b''))
        status, headers, _ = self.exchange(request('/', method='DELETE'))
        self.assertEqual(status, 405)
        self.assertIn(b'GET', headers.get(b'allow', b''))

    def test_upload_download_delete_binary(self):
        body = b'a\x00b\xff\r\n' * 100
        status, headers, _ = self.exchange(request('/upload', 'POST', body))
        self.assertEqual(status, 201)
        path = headers[b'location'].decode()
        self.assertEqual(self.exchange(request(path))[2], body)
        self.assertEqual(self.exchange(request(path, 'DELETE'))[0], 204)
        self.assertEqual(self.exchange(request(path))[0], 404)
        self.assertEqual(self.exchange(request(path, 'DELETE'))[0], 404)

    def test_body_limits(self):
        for host, size, expected in [('local.test', 1024, 201), ('local.test', 1025, 413),
                                     ('second.test', 8, 201), ('second.test', 9, 413)]:
            with self.subTest(host=host, size=size):
                self.assertEqual(self.exchange(request('/upload', 'POST', b'x' * size, host=host))[0], expected)

    def test_oversized_body_rejected_before_body_arrives(self):
        raw = b'POST /upload HTTP/1.1\r\nHost: local.test\r\nContent-Length: 1025\r\n\r\n'
        self.assertEqual(self.exchange(raw)[0], 413)

    def test_fragmented_request(self):
        data = request('/upload', 'POST', b'fragment\x00body')
        with self.connect() as sock:
            for byte in data:
                sock.sendall(bytes([byte]))
                time.sleep(.001)
            status, headers, _ = Wire(sock).read()
        self.assertEqual(status, 201)
        self.assertEqual(self.exchange(request(headers[b'location'].decode()))[2], b'fragment\x00body')

    def test_pipelining_and_connection_close(self):
        with self.connect() as sock:
            wire = Wire(sock)
            sock.sendall(request(close=False) + request('/missing', close=False) + request())
            for code in (200, 404, 200):
                self.assertEqual(wire.read()[0], code)
            self.assertEqual(wire.buffer, b'')
            self.assertEqual(sock.recv(1), b'')

    def test_keep_alive_reuse(self):
        with self.connect() as sock:
            wire = Wire(sock)
            for _ in range(50):
                sock.sendall(request(close=False))
                self.assertEqual(wire.read()[2], b'primary-index\n')

    def test_chunked_pipeline(self):
        raw = (b'POST /upload HTTP/1.1\r\nHost: local.test\r\nTransfer-Encoding: chunked\r\n\r\n'
               b'3\r\na\x00b\r\n2\r\ncd\r\n0\r\n\r\n')
        with self.connect() as sock:
            sock.sendall(raw + request())
            wire = Wire(sock)
            status, headers, _ = wire.read()
            self.assertEqual(status, 201)
            self.assertEqual(wire.read()[2], b'primary-index\n')
        self.assertEqual(self.exchange(request(headers[b'location'].decode()))[2], b'a\x00bcd')

    def test_malformed_headers(self):
        cases = [b'', b'Host: a\r\nHost: b\r\n', b'Host: \r\n',
                 b'Host: a\r\nBroken\r\n', b'Host: a\r\nContent-Length: -1\r\n',
                 b'Host: a\r\nContent-Length: 999999999999999999999999\r\n',
                 b'Host: a\r\nContent-Length: 0\r\nContent-Length: 1\r\n',
                 b'Host: a\r\nContent-Length: 0\r\nTransfer-Encoding: chunked\r\n']
        for headers in cases:
            with self.subTest(headers=headers):
                with self.connect() as sock:
                    sock.sendall(b'GET / HTTP/1.1\r\n' + headers + b'\r\n')
                    self.assertEqual(Wire(sock).read()[0], 400)
                    self.assertEqual(sock.recv(1), b'')

    def test_whitespace_before_header_colon(self):
        self.assertEqual(self.exchange(b'GET / HTTP/1.1\r\nHost : local.test\r\nConnection: close\r\n\r\n')[0], 400)

    def test_invalid_chunks(self):
        for chunk in (b'Z\r\n', b'1\r\nxXX', b'FFFFFFFFFFFFFFFFFFFFFFFF\r\n'):
            with self.subTest(chunk=chunk):
                raw = b'POST /upload HTTP/1.1\r\nHost: local.test\r\nTransfer-Encoding: chunked\r\n\r\n'
                self.assertEqual(self.exchange(raw + chunk)[0], 400)

    def test_signed_chunk_size_rejected(self):
        raw = b'POST /upload HTTP/1.1\r\nHost: local.test\r\nTransfer-Encoding: chunked\r\n\r\n'
        self.assertEqual(self.exchange(raw + b'+1\r\nx\r\n0\r\n\r\n')[0], 400)

    def test_chunked_body_limit(self):
        raw = b'POST /upload HTTP/1.1\r\nHost: local.test\r\nTransfer-Encoding: chunked\r\n\r\n'
        self.assertEqual(self.exchange(raw + b'401\r\n')[0], 413)

    def test_header_limit(self):
        self.assertEqual(self.exchange(request(headers=b'X-Large: ' + b'x' * 32768 + b'\r\n'))[0], 431)

    def test_path_escape_and_invalid_encoding(self):
        for path in ('/../secret.txt', '/%2e%2e/secret.txt', '/escape', '/bad%00name', '/bad%GG'):
            with self.subTest(path=path):
                status, _, body = self.exchange(request(path))
                self.assertIn(status, (400, 403, 404))
                self.assertNotIn(b'OUTSIDE-ROOT-SECRET', body)

    def test_slow_clients_do_not_block_others(self):
        sockets = []
        try:
            for _ in range(20):
                sock = self.connect()
                sockets.append(sock)
                sock.sendall(b'POST /upload HTTP/1.1\r\nHost: local.test\r\nContent-Length: 100\r\n\r\nx')
            self.assertEqual(self.exchange(request())[0], 200)
        finally:
            for sock in sockets:
                sock.close()

    def test_slow_reader_does_not_block_others(self):
        with self.connect() as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
            sock.sendall(request('/large.bin'))
            time.sleep(.1)
            self.assertEqual(self.exchange(request())[2], b'primary-index\n')

    def test_interrupted_upload_creates_no_file(self):
        before = set(self.uploads.iterdir())
        with self.connect() as sock:
            sock.sendall(b'POST /upload HTTP/1.1\r\nHost: local.test\r\nContent-Length: 100\r\n\r\npartial')
        time.sleep(.1)
        self.assertEqual(self.exchange(request())[0], 200)
        self.assertEqual(set(self.uploads.iterdir()), before)

    def test_half_closed_client_receives_response(self):
        with self.connect() as sock:
            sock.sendall(request())
            sock.shutdown(socket.SHUT_WR)
            self.assertEqual(Wire(sock).read()[2], b'primary-index\n')

    def test_concurrent_clients(self):
        def fetch(_):
            return self.exchange(request())[2]
        with concurrent.futures.ThreadPoolExecutor(max_workers=24) as pool:
            results = list(pool.map(fetch, range(240)))
        self.assertTrue(all(body == b'primary-index\n' for body in results))

    def churn(self, count):
        for i in range(count):
            with self.connect() as sock:
                if i % 4 == 0:
                    sock.sendall(request())
                    self.assertEqual(Wire(sock).read()[0], 200)
                elif i % 4 == 1:
                    sock.sendall(b'GET / HTTP/1.1\r\nHost:')
                elif i % 4 == 2:
                    sock.sendall(b'POST /upload HTTP/1.1\r\nHost: local.test\r\nContent-Length: 1024\r\n\r\npartial')
                else:
                    sock.sendall(request('/large.bin'))
                    sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
        self.assertEqual(self.exchange(request())[0], 200)

    def fd_count(self):
        proc = Path(f'/proc/{self.process.pid}/fd')
        if proc.exists():
            return len(list(proc.iterdir()))
        lsof = shutil.which('lsof')
        if not lsof:
            self.skipTest('fd measurement needs /proc or lsof')
        result = subprocess.run([lsof, '-nP', '-a', '-p', str(self.process.pid), '-Ff'],
                                capture_output=True, text=True, timeout=10)
        descriptors = [line[1:] for line in result.stdout.splitlines()
                       if line.startswith('f') and line[1:].isdigit()]
        if result.returncode or not descriptors:
            self.skipTest('lsof cannot inspect server descriptors: ' + result.stderr.strip())
        return len(descriptors)

    def test_resource_fd_cleanup(self):
        self.churn(40)
        time.sleep(.2)
        before = self.fd_count()
        self.churn(400)
        deadline = time.monotonic() + 5
        after = self.fd_count()
        while after > before and time.monotonic() < deadline:
            time.sleep(.1)
            after = self.fd_count()
        print(f'\nFD count: before={before}, after={after}', flush=True)
        self.assertLessEqual(after, before, 'server retains descriptors after clients disconnect')

    def test_resource_memory_growth(self):
        def rss():
            result = subprocess.run(['ps', '-o', 'rss=', '-p', str(self.process.pid)],
                                    capture_output=True, text=True, timeout=5)
            if result.returncode or not result.stdout.strip():
                self.skipTest('RSS measurement unavailable: ' + result.stderr.strip())
            return int(result.stdout.strip())
        self.churn(80)  # Warm allocator and large-response buffers first.
        time.sleep(.2)
        samples = [rss()]
        for _ in range(3):
            self.churn(160)
            time.sleep(.2)
            samples.append(rss())
        print(f'\nRSS samples (KiB): {samples}', flush=True)
        allowance = int(os.environ.get('WEBSERV_RSS_ALLOWANCE_KIB', '32768'))
        self.assertLessEqual(samples[-1] - samples[0], allowance,
                             'sustained memory growth exceeds allowance (not a precise leak detector)')

    @unittest.skipUnless(os.environ.get('WEBSERV_NATIVE_LEAKS') == '1', 'set WEBSERV_NATIVE_LEAKS=1 for macOS leaks')
    def test_native_leaks(self):
        leaks = shutil.which('leaks')
        if sys.platform != 'darwin' or not leaks:
            self.skipTest('native live-process leak scan requires macOS leaks')
        self.churn(400)
        time.sleep(.2)
        result = subprocess.run([leaks, '--quiet', str(self.process.pid)],
                                capture_output=True, text=True, timeout=45)
        output = result.stdout + result.stderr
        print('\n' + output, flush=True)
        self.assertEqual(result.returncode, 0, 'leaks found a leak or could not inspect the process')
        self.assertRegex(output, r'0 leaks for 0 total leaked bytes', 'missing successful leak scan summary')

    @unittest.skipUnless(os.environ.get('WEBSERV_SLOW_TESTS') == '1', 'set WEBSERV_SLOW_TESTS=1 for 35-second timeout test')
    def test_idle_and_partial_request_timeout(self):
        sockets = [self.connect(), self.connect()]
        try:
            sockets[1].sendall(b'GET / HTTP/1.1\r\nHost:')
            deadline = time.monotonic() + 35
            for sock in sockets:
                sock.settimeout(max(.1, deadline - time.monotonic()))
                self.assertEqual(sock.recv(1), b'')
            self.assertEqual(self.exchange(request())[0], 200)
        finally:
            for sock in sockets:
                sock.close()


if __name__ == '__main__':
    unittest.main(verbosity=2)
