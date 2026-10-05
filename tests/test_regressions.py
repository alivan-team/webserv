#!/usr/bin/env python3
"""HTTP/startup regressions. Run from the project root after make."""
import http.client
import pathlib
import socket
import subprocess
import sys
import tempfile
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]


def free_port():
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        return sock.getsockname()[1]


def expect_status(port, method, path, expected, body=None, headers=None):
    conn = http.client.HTTPConnection('127.0.0.1', port, timeout=5)
    try:
        conn.request(method, path, body=body, headers=headers or {})
        response = conn.getresponse()
        data = response.read()
        assert response.status == expected, (method, path, response.status, expected, data)
        return response, data
    finally:
        conn.close()


def main():
    with tempfile.TemporaryDirectory(prefix='webserv-regressions-') as temporary:
        base = pathlib.Path(temporary)
        scripts = base / 'cgi'
        uploads = base / 'uploads'
        scripts.mkdir()
        uploads.mkdir()
        (scripts / 'echo.py').write_text(
            "import sys\nbody = sys.stdin.buffer.read()\n"
            "sys.stdout.buffer.write(b'Content-Type: application/octet-stream\\n\\n' + "
            "(body or b'CGI OK'))\n")
        (scripts / 'directory.py').mkdir()
        (scripts / 'escape.py').symlink_to(ROOT / 'main.cpp')
        interpreter = pathlib.Path(sys.executable).resolve()
        port = free_port()
        config = base / 'valid.conf'
        config.write_text(f'''server {{
listen 127.0.0.1:{port};
server_name localhost;
root {base};
location /cgi-bin {{
root {scripts}; allow_methods GET POST;
cgi_extension .py; cgi_path {interpreter};
}}
location /plain {{ allow_methods GET POST; }}
location /get-only {{ allow_methods GET; }}
location /upload {{ allow_methods POST; upload_store {uploads}; }}
location /redirect {{ return 301 /new; }}
}}
''')
        with (base / 'server.log').open('w+') as log:
            process = subprocess.Popen([str(ROOT / 'webserv'), str(config)], cwd=ROOT,
                                       stdout=log, stderr=log)
            try:
                deadline = time.monotonic() + 5
                while True:
                    if process.poll() is not None:
                        log.seek(0)
                        raise AssertionError('server failed to start: ' + log.read())
                    try:
                        with socket.create_connection(('127.0.0.1', port), timeout=.2):
                            break
                    except OSError:
                        if time.monotonic() >= deadline:
                            raise
                        time.sleep(.05)
                for method in ('GET', 'POST'):
                    for name in ('missing.py', 'directory.py', 'missing%2Epy'):
                        expect_status(port, method, '/cgi-bin/' + name, 404)
                    expect_status(port, method, '/cgi-bin/escape.py', 403)
                for method in ('GET', 'POST'):
                    _, data = expect_status(port, method, '/cgi-bin/echo.py', 200)
                    assert data == b'CGI OK'
                binary = b'A\x00B\xffC\r\n'
                _, data = expect_status(port, 'POST', '/cgi-bin/echo.py', 200, binary)
                assert data == binary
                expect_status(port, 'POST', '/plain', 403, b'hello')
                expect_status(port, 'POST', '/get-only', 405, b'hello')
                expect_status(port, 'POST', '/redirect', 301, b'hello')
                expect_status(port, 'POST', '/upload', 201, binary)
                files = list(uploads.iterdir())
                assert len(files) == 1 and files[0].read_bytes() == binary
                multipart = (b'--boundary\r\nContent-Disposition: form-data; name="file"; '
                             b'filename="BiNaRy.dat"\r\nContent-Type: application/octet-stream\r\n\r\n'
                             + binary + b'\r\n--boundary--\r\n')
                expect_status(port, 'POST', '/upload', 201, multipart,
                              {'Content-Type': 'multipart/form-data; boundary=boundary'})
                assert (uploads / 'BiNaRy.dat').read_bytes() == binary
                print('PASS HTTP: CGI 404/403, working CGI GET/POST, absent upload_store, uploads')
            finally:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
        non_executable = base / 'non-executable'
        non_executable.write_text('not executable\n')
        non_executable.chmod(0o600)
        cases = [
            'cgi_extension .py;',
            f'cgi_path {interpreter};',
            f'cgi_extension .py .sh; cgi_path {interpreter};',
            f'cgi_extension .py; cgi_path {base / "absent"};',
            f'cgi_extension .py; cgi_path {base};',
            f'cgi_extension .py; cgi_path {non_executable};',
        ]
        for directives in cases:
            config.write_text(f'''server {{ listen 127.0.0.1:{port}; location / {{ allow_methods GET; }} }}
server {{ listen 127.0.0.1:{port + 1 if port < 65535 else port - 1};
location /cgi-bin {{ allow_methods GET POST; {directives} }} }}''')
            result = subprocess.run([str(ROOT / 'webserv'), str(config)], cwd=ROOT,
                                    capture_output=True, text=True, timeout=5)
            assert result.returncode == 1, (directives, result.returncode)
            assert 'cgi' in result.stderr.lower(), (directives, result.stderr)
        print('PASS startup: 6 invalid CGI configurations rejected before serving requests')


if __name__ == '__main__':
    main()
