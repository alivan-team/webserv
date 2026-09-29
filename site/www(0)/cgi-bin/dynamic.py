import os
import sys
from urllib.parse import parse_qs

query = parse_qs(os.environ.get("QUERY_STRING", ""))

name = query.get("name", ["Guest"])[0]

print("Status: 200 OK")
print("Content-Type: text/html")
print()

print(f"""<!DOCTYPE html>
<html>
<head>
    <title>CGI Demo</title>
</head>
<body>
    <h1>Hello, {name}!</h1>
    <p>This HTML page was generated dynamically by Python CGI.</p>
    <p>Request method: {os.environ.get("REQUEST_METHOD", "")}</p>
</body>
</html>
""")