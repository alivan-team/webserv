#!/usr/bin/env python3

import os
import time

os.close(0)

time.sleep(2)

print("Content-Type: text/plain")
print()
print("This should never become the HTTP response")