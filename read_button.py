#!/usr/bin/env python

import time
import os.path


prefix = "/sys/class/gpio"


def to_file(fname: str, val: int):
    with open(os.path.join(prefix, fname), "w") as f:
        f.write(f"{val}\n")


def from_file(fname: str) -> str:
    with open(os.path.join(prefix, fname), "r") as f:
        return f.read().strip()


base = 454
SHUTDOWN = base + 5
BOOT = base + 12
try:
    to_file("export", f"{SHUTDOWN}")
except OSError:
    pass
try:
    to_file("export", f"{BOOT}")
except OSError:
    pass
to_file(f"gpio{SHUTDOWN}/direction", "in")
to_file(f"gpio{BOOT}/direction", "out")
to_file(f"gpio{BOOT}/value", "1")

c = None
while True:
    n = from_file(f"gpio{SHUTDOWN}/value")
    if n != c:
        print(n)
        c = n
    time.sleep(0.1)
