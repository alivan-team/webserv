
#!/usr/bin/env python3

import os
import sys
import html
from datetime import datetime
from urllib.parse import parse_qs

# Read CGI request information
method = os.environ.get("REQUEST_METHOD", "GET")
query = os.environ.get("QUERY_STRING", "")

params = parse_qs(query)

if method == "POST":
    length = int(os.environ.get("CONTENT_LENGTH", "0"))
    body = sys.stdin.read(length)
    params = parse_qs(body)

name = params.get("name", ["Webserv Developer"])[0]
name = html.escape(name)

current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
python_version = html.escape(sys.version.split()[0])
server_name = html.escape(os.environ.get("SERVER_NAME", "localhost"))

page = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv Dynamic CGI</title>
    <style>
        * {{ box-sizing: border-box; }}

        body {{
            margin: 0;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            background: #0f172a;
            color: #e2e8f0;
            font-family: Arial, sans-serif;
        }}

        .card {{
            width: 90%;
            max-width: 650px;
            padding: 40px;
            border-radius: 20px;
            background: #1e293b;
            box-shadow: 0 20px 60px rgba(0,0,0,.3);
        }}

        h1 {{ color: #38bdf8; }}

        .status {{ color: #4ade80; }}

        .info {{
            padding: 15px;
            margin: 12px 0;
            background: #334155;
            border-radius: 8px;
        }}

        input, button {{
            padding: 12px;
            border: none;
            border-radius: 6px;
            font-size: 16px;
        }}

        input {{ width: 65%; }}

        button {{
            background: #38bdf8;
            cursor: pointer;
        }}
    </style>
</head>
<body>
    <div class="card">
        <h1>Webserv CGI Engine</h1>
        <h2>Hello, {name}!</h2>

        <p class="status">● Python CGI execution successful</p>

        <div class="info">
            <strong>Current server time:</strong> {current_time}
        </div>

        <div class="info">
            <strong>HTTP method:</strong> {html.escape(method)}
        </div>

        <div class="info">
            <strong>Python version:</strong> {python_version}
        </div>

        <div class="info">
            <strong>Server name:</strong> {server_name}
        </div>

        <h3>Test dynamic content</h3>

        <form method="GET" action="/cgi-bin/dynamic.py">
            <input type="text" name="name"
                   placeholder="Enter your name">
            <button type="submit">Generate</button>
        </form>

        <p>Generated dynamically by Python, served by our C++ Webserv.</p>
    </div>
</body>
</html>
"""

# CGI response headers
sys.stdout.write("Content-Type: text/html; charset=utf-8\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(page)
sys.stdout.flush()
