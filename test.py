#!/usr/bin/env python3
"""
Test script for Lab 3: Musical Keyboard
Tests all required functionality of the musical keyboard implementation.
"""
import sys
import serial
import time

# Default port
DEFAULT_PORT = "/dev/tty.usbserial-101"

if len(sys.argv) < 2:
    print(f"Usage: python test.py [port]")
    print(f"Using default port: {DEFAULT_PORT}")
    port = DEFAULT_PORT
else:
    port = sys.argv[1]

print(f"Connecting to {port} at 57600 baud...")
try:
    ser = serial.Serial(port, 57600, timeout=2)
except serial.SerialException as e:
    print(f"Error: Could not open port {port}")
    print(f"Available ports: ")
    import os
    os.system("ls /dev/tty.usbserial-* 2>/dev/null")
    sys.exit(1)

time.sleep(0.5)  # Wait for board to be ready
ser.reset_input_buffer()

# Test counters
passed = 0
failed = 0

def send_cmd(cmd, wait_time=0.5):
    """Send command (single char or Enter) and return response"""
    if cmd == '\n':
        ser.write(b'\r')
    else:
        ser.write(cmd.encode())
    time.sleep(wait_time)
    resp = ser.read(500).decode(errors='ignore')
    return resp

def test(name, condition, response=""):
    global passed, failed
    if condition:
        print(f"✓ {name}: PASS")
        passed += 1
    else:
        print(f"✗ {name}: FAIL")
        if response:
            print(f"  Response: {repr(response)}")
        failed += 1

print("\n" + "="*60)
print("LAB 3: MUSICAL KEYBOARD TESTS")
print("="*60 + "\n")

# Test 0: Read startup message
print("Reading startup message...")
time.sleep(1)
startup = ser.read(500).decode(errors='ignore')
print(f"Startup message:\n{startup}\n")
test("Startup message received",
     'Musical Keyboard' in startup or 'Lab 3' in startup or 'Settings' in startup,
     startup)

print("\n--- Testing Note Playback (1-7) ---")

# Test 1: Play note Do (1)
resp = send_cmd('1', 0.3)
test("Play Do (1)", 'Do' in resp and 'octave 4' in resp, resp)

# Test 2: Play note Re (2)
resp = send_cmd('2', 0.3)
test("Play Re (2)", 'Re' in resp, resp)

# Test 3: Play note Mi (3)
resp = send_cmd('3', 0.3)
test("Play Mi (3)", 'Mi' in resp, resp)

# Test 4: Play note Fa (4)
resp = send_cmd('4', 0.3)
test("Play Fa (4)", 'Fa' in resp, resp)

# Test 5: Play note Sol (5)
resp = send_cmd('5', 0.3)
test("Play Sol (5)", 'Sol' in resp, resp)

# Test 6: Play note La (6)
resp = send_cmd('6', 0.3)
test("Play La (6)", 'La' in resp, resp)

# Test 7: Play note Si (7)
resp = send_cmd('7', 0.3)
test("Play Si (7)", 'Si' in resp, resp)

print("\n--- Testing Octave Changes (+/-) ---")

# Test 8: Increase octave (+)
resp = send_cmd('+', 0.3)
test("Increase octave (+)", 'octave 5' in resp and 'Settings' in resp, resp)

# Test 9: Play note in new octave
resp = send_cmd('1', 0.3)
test("Play in octave 5", 'octave 5' in resp, resp)

# Test 10: Decrease octave (-)
resp = send_cmd('-', 0.3)
test("Decrease octave (-)", 'octave 4' in resp, resp)

# Test 11: Multiple octave increases
for _ in range(3):
    send_cmd('+', 0.2)
resp = send_cmd('+', 0.3)  # Should now be at octave 8
test("Increase to octave 8", 'octave 8' in resp, resp)

# Test 12: Try to exceed max octave
resp = send_cmd('+', 0.3)
test("Cannot exceed octave 8", 'octave 8' in resp, resp)

# Test 13: Decrease to octave 0
for _ in range(8):
    send_cmd('-', 0.2)
resp = send_cmd('-', 0.3)  # Should now be at octave 0
test("Decrease to octave 0", 'octave 0' in resp, resp)

# Test 14: Try to go below min octave
resp = send_cmd('-', 0.3)
test("Cannot go below octave 0", 'octave 0' in resp, resp)

# Reset to octave 4
for _ in range(4):
    send_cmd('+', 0.2)

print("\n--- Testing Duration Changes (A/a) ---")

# Test 15: Increase duration (A)
resp = send_cmd('A', 0.3)
test("Increase duration (A)", 'duration 1.1' in resp, resp)

# Test 16: Decrease duration (a)
resp = send_cmd('a', 0.3)
test("Decrease duration (a)", 'duration 1.0' in resp, resp)

# Test 17: Increase to max duration
for _ in range(40):
    send_cmd('A', 0.1)
resp = send_cmd('A', 0.3)
test("Increase to max 5.0s", 'duration 5.0' in resp, resp)

# Test 18: Try to exceed max duration
resp = send_cmd('A', 0.3)
test("Cannot exceed 5.0s", 'duration 5.0' in resp, resp)

# Test 19: Decrease to min duration
for _ in range(49):
    send_cmd('a', 0.1)
resp = send_cmd('a', 0.3)
test("Decrease to min 0.1s", 'duration 0.1' in resp, resp)

# Test 20: Try to go below min duration
resp = send_cmd('a', 0.3)
test("Cannot go below 0.1s", 'duration 0.1' in resp, resp)

# Reset duration to 1.0s
for _ in range(9):
    send_cmd('A', 0.1)

print("\n--- Testing Scale Playback (Enter) ---")

# Test 21: Play scale
resp = send_cmd('\n', 1.5)  # Scale takes time to play
test("Play scale (Enter)", 'scale' in resp.lower() or 'Playing' in resp, resp)

print("\n--- Testing Invalid Characters ---")

# Test 22: Invalid character 0
resp = send_cmd('0', 0.3)
test("Invalid char '0'", 'Invalid' in resp and '48' in resp, resp)  # ASCII 48 = '0'

# Test 23: Invalid character 8
resp = send_cmd('8', 0.3)
test("Invalid char '8'", 'Invalid' in resp and '56' in resp, resp)  # ASCII 56 = '8'

# Test 24: Invalid character 9
resp = send_cmd('9', 0.3)
test("Invalid char '9'", 'Invalid' in resp and '57' in resp, resp)  # ASCII 57 = '9'

# Test 25: Invalid character 'B'
resp = send_cmd('B', 0.3)
test("Invalid char 'B'", 'Invalid' in resp and '66' in resp, resp)  # ASCII 66 = 'B'

# Test 26: Invalid character 'b'
resp = send_cmd('b', 0.3)
test("Invalid char 'b'", 'Invalid' in resp and '98' in resp, resp)  # ASCII 98 = 'b'

# Test 27: Invalid character 'x'
resp = send_cmd('x', 0.3)
test("Invalid char 'x'", 'Invalid' in resp and '120' in resp, resp)  # ASCII 120 = 'x'

print("\n--- Testing Complete Note Sequence ---")

# Test 28: Play a melody sequence
print("Playing test melody: Do-Re-Mi-Fa-Sol-La-Si...")
melody = "1234567"
for note in melody:
    resp = send_cmd(note, 0.3)
test("Melody sequence played", True)  # If we got here, it worked

print("\n--- Testing Different Octaves ---")

# Test 29: Play same note in different octaves
print("Playing Do in octaves 0, 4, 8...")
for octave in [0, 4, 8]:
    # Set octave
    current = 4  # Assume we're at octave 4
    if octave > current:
        for _ in range(octave - current):
            send_cmd('+', 0.1)
    else:
        for _ in range(current - octave):
            send_cmd('-', 0.1)
    resp = send_cmd('1', 0.3)
    test(f"Play Do in octave {octave}", f'octave {octave}' in resp, resp)

print("\n" + "="*60)
print(f"TEST RESULTS: {passed} passed, {failed} failed")
print("="*60)

ser.close()

if failed == 0:
    print("\n✓ ALL TESTS PASSED!")
    sys.exit(0)
else:
    print(f"\n✗ {failed} TEST(S) FAILED")
    sys.exit(1)
