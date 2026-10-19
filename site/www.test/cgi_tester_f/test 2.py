#!/usr/bin/env python3
import json
import os
import sys

method = os.environ.get("REQUEST_METHOD", "")
content_length = os.environ.get("CONTENT_LENGTH", "0")
try:
    length = int(content_length or "0")
except ValueError:
    length = 0
body = sys.stdin.read(length) if length > 0 else ""

payload = {
    "cgi": "working",
    "request_method": method,
    "query_string": os.environ.get("QUERY_STRING", ""),
    "script_name": os.environ.get("SCRIPT_NAME", ""),
    "server_protocol": os.environ.get("SERVER_PROTOCOL", ""),
    "server_name": os.environ.get("SERVER_NAME", ""),
    "server_port": os.environ.get("SERVER_PORT", ""),
    "content_type": os.environ.get("CONTENT_TYPE", ""),
    "content_length": content_length,
    "x_evaluation_demo": os.environ.get("HTTP_X_EVALUATION_DEMO", ""),
    "body": body,
    "working_directory": os.getcwd(),
}

print("Content-Type: application/json")
print("X-CGI-Demo: webserv")
print()
print(json.dumps(payload, indent=2))
