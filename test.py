#!/usr/bin/env python3
import sys
import serial

if len(sys.argv) < 2:
    print("Usage: python test.py /dev/tty.usbserial-1101")
    sys.exit(1)

port = sys.argv[1]
ser = serial.Serial(port, 115200, timeout=1)

# Test 1: Echo
ser.write(b'A')
resp = ser.read(1)
if resp == b'A':
    print("Echo test: PASS")
else:
    print(f"Echo test: FAIL (got {resp})")

# Test 2: Hello command
ser.write(b'!')
resp = ser.read(20)
if b"Hello, world!" in resp:
    print("Hello test: PASS")
else:
    print(f"Hello test: FAIL (got {resp})")

ser.close()
