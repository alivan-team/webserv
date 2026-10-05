#!/usr/bin/env python3

import sys

body = sys.stdin.read()

print("Content-Type: text/plain")
print()
print(body)