import os
import sys

# CGI response headers
print("Status: 200 OK")
print("Content-Type: text/plain")
print("X-Test-Header: Hello-From-Python")
print()

# CGI response body
print("Hello from CGI!")
print("Method:", os.environ.get("REQUEST_METHOD", ""))
print("Query:", os.environ.get("QUERY_STRING", ""))
print("Content-Type received:", os.environ.get("CONTENT_TYPE", ""))
print("Content-Length received:", os.environ.get("CONTENT_LENGTH", ""))

body = sys.stdin.read()

if body:
    print("Body:", body)