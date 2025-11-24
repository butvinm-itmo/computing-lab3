#!/usr/bin/env python3
import sys
import serial
import time

if len(sys.argv) < 2:
    print("Usage: python test.py /dev/tty.usbserial-101")
    sys.exit(1)

port = sys.argv[1]
ser = serial.Serial(port, 57600, timeout=2)
time.sleep(0.5)  # Wait for board to be ready
ser.reset_input_buffer()

def send_cmd(cmd):
    """Send command and return response"""
    ser.write((cmd + '\r').encode())
    time.sleep(0.3)
    resp = ser.read(200).decode(errors='ignore')
    return resp

def test(name, condition):
    if condition:
        print(f"{name}: PASS")
    else:
        print(f"{name}: FAIL")

# Read startup message
startup = ser.read(100).decode(errors='ignore')
print(f"Startup: {repr(startup)}")

# Test 1: Status query
resp = send_cmd('?')
test("Status query (?)", 'red' in resp.lower() or 'green' in resp.lower() or 'yellow' in resp.lower())

# Test 2: Set mode 1
resp = send_cmd('set mode 1')
test("Set mode 1", 'OK' in resp)

# Test 3: Set mode 2
resp = send_cmd('set mode 2')
test("Set mode 2", 'OK' in resp)

# Test 4: Set timeout
resp = send_cmd('set timeout 5')
test("Set timeout 5", 'OK' in resp)

# Test 5: Check status reflects new timeout
resp = send_cmd('?')
test("Timeout updated", 'timeout 5' in resp)

# Test 6: Set interrupts off
resp = send_cmd('set interrupts off')
test("Set interrupts off", 'OK' in resp)

# Test 7: Check polling mode
resp = send_cmd('?')
test("Polling mode (P)", ' P' in resp)

# Test 8: Set interrupts on
resp = send_cmd('set interrupts on')
test("Set interrupts on", 'OK' in resp)

# Test 9: Check IRQ mode
resp = send_cmd('?')
test("IRQ mode (I)", ' I' in resp)

# Test 10: Unknown command
resp = send_cmd('invalid command')
test("Unknown command error", 'ERROR' in resp)

ser.close()
print("\nAll tests completed.")
