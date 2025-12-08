#!/usr/bin/env python3
"""
Test script for Lab 4: I2C Matrix Keyboard Musical Keyboard
Interactive test suite - prompts user to press buttons on matrix keypad.
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

def wait_for_input(prompt, timeout=5, is_side_button=False):
    """Prompt user to press a button and capture UART output"""
    print(f"\n>>> {prompt}")
    if is_side_button:
        input("    Press ENTER when ready, then press the SIDE BUTTON on panel...")
    else:
        input("    Press ENTER when ready, then press the button on keypad...")
    ser.reset_input_buffer()
    time.sleep(0.1)

    # Wait for response
    start = time.time()
    response = ""
    while time.time() - start < timeout:
        if ser.in_waiting:
            chunk = ser.read(ser.in_waiting).decode(errors='ignore')
            response += chunk
            time.sleep(0.2)  # Wait a bit more in case there's more data

    return response.strip()

def test(name, condition, response=""):
    global passed, failed
    if condition:
        print(f"    ✓ {name}: PASS")
        passed += 1
    else:
        print(f"    ✗ {name}: FAIL")
        if response:
            print(f"      Response: {repr(response)}")
        failed += 1

print("\n" + "="*70)
print("LAB 4: I2C MATRIX KEYBOARD TESTS")
print("="*70 + "\n")

print("This test suite is INTERACTIVE.")
print("You will be prompted to press buttons on the matrix keypad.")
print("Make sure the device is in the correct mode for each test.")
print("\nPress ENTER to begin...")
input()

# Read initial message (if any)
time.sleep(1)
initial = ser.read(ser.in_waiting).decode(errors='ignore')
if initial:
    print("\nInitial message from device:")
    print(initial)
else:
    print("\nNo initial message (device ready)")

print("\n" + "="*70)
print("PART 1: MODE SWITCHING")
print("="*70)
print("Note: Device starts in MUSICAL mode by default")
print("IMPORTANT: The SIDE BUTTON is on the panel (PC15), NOT on the matrix keypad!")

# Test 1: Switch to test mode (to verify we're starting from musical mode)
resp = wait_for_input("Switch to TEST mode", is_side_button=True)
test("Switch to test mode",
     "test" in resp.lower() and "mode" in resp.lower(),
     resp)

print("\n" + "="*70)
print("PART 2: KEYBOARD TEST MODE (Testing all 12 buttons)")
print("="*70)

# Test buttons 1-12 in test mode
for btn in range(1, 13):
    resp = wait_for_input(f"Press button {btn} on the keypad")
    test(f"Button {btn} detected",
         f"Button pressed: {btn}" in resp or f"pressed: {btn}" in resp,
         resp)

print("\n" + "="*70)
print("PART 3: DEBOUNCE TEST")
print("="*70)

# Test debounce - hold button should only register once
resp = wait_for_input("Press and HOLD button 1 for 2 seconds", timeout=3)
count = resp.count("Button pressed:")
test("Debounce protection (no repeat on hold)",
     count == 1,
     f"Expected 1 press, got {count} presses. Response: {resp}")

print("\n" + "="*70)
print("PART 4: ANTI-GHOSTING TEST")
print("="*70)

# Test anti-ghosting - press two buttons simultaneously
resp = wait_for_input("Press TWO buttons SIMULTANEOUSLY (e.g., 1 and 2)", timeout=3)
test("Anti-ghosting (no detection on multiple press)",
     "Button pressed:" not in resp or resp.count("Button pressed:") == 0,
     resp)

print("\n" + "="*70)
print("PART 5: SWITCH TO MUSICAL MODE")
print("="*70)

# Switch back to musical mode
resp = wait_for_input("Switch back to MUSICAL mode", is_side_button=True)
test("Switch to musical mode",
     "musical" in resp.lower() and "mode" in resp.lower(),
     resp)

print("\n" + "="*70)
print("PART 6: MUSICAL MODE - NOTE PLAYBACK (Buttons 1-7)")
print("="*70)

notes = ["Do", "Re", "Mi", "Fa", "Sol", "La", "Si"]
for i, note in enumerate(notes, 1):
    resp = wait_for_input(f"Press button {i} to play note {note}")
    test(f"Play note {note} (button {i})",
         note in resp and "Playing" in resp and "octave" in resp,
         resp)

print("\n" + "="*70)
print("PART 7: MUSICAL MODE - OCTAVE CONTROL (Buttons 8-9)")
print("="*70)

# Test octave up (button 8)
resp = wait_for_input("Press button 8 to increase octave")
test("Increase octave (button 8)",
     "octave 5" in resp and "Settings" in resp,
     resp)

# Test octave down (button 9)
resp = wait_for_input("Press button 9 to decrease octave")
test("Decrease octave (button 9)",
     "octave 4" in resp and "Settings" in resp,
     resp)

# Test max octave
print("\n>>> Rapidly press button 8 FOUR times to reach octave 8...")
input("    Press ENTER when ready...")
ser.reset_input_buffer()
time.sleep(0.5)
for _ in range(4):
    input("    Press button 8 now...")
    time.sleep(0.3)
resp = ser.read(ser.in_waiting).decode(errors='ignore')
test("Reach max octave 8",
     "octave 8" in resp,
     resp)

# Try to exceed max
resp = wait_for_input("Press button 8 again (should stay at octave 8)")
test("Cannot exceed octave 8",
     "octave 8" in resp,
     resp)

# Reset to octave 4
print("\n>>> Rapidly press button 9 FOUR times to return to octave 4...")
input("    Press ENTER when ready...")
for _ in range(4):
    input("    Press button 9 now...")
    time.sleep(0.3)
time.sleep(0.3)
ser.reset_input_buffer()

print("\n" + "="*70)
print("PART 8: MUSICAL MODE - DURATION CONTROL (Buttons 10-11)")
print("="*70)

# Test duration up (button 10)
resp = wait_for_input("Press button 10 to increase duration")
test("Increase duration (button 10)",
     "duration 1.1" in resp or "duration 1.2" in resp,
     resp)

# Test duration down (button 11)
resp = wait_for_input("Press button 11 to decrease duration")
test("Decrease duration (button 11)",
     "duration 1.0" in resp or "duration 1.1" in resp,
     resp)

print("\n" + "="*70)
print("PART 9: MUSICAL MODE - SCALE PLAYBACK (Button 12)")
print("="*70)

# Test scale playback
resp = wait_for_input("Press button 12 to play scale", timeout=8)
test("Play scale (button 12)",
     "scale" in resp.lower() and "Playing" in resp,
     resp)

print("\n" + "="*70)
print("PART 10: FINAL INTEGRATION TEST")
print("="*70)

# Test quick sequence
print("\n>>> Quick test: Press buttons 1, 2, 3 in sequence...")
input("    Press ENTER when ready...")
ser.reset_input_buffer()
time.sleep(0.1)
for btn in [1, 2, 3]:
    input(f"    Press button {btn} now...")
    time.sleep(0.3)
time.sleep(0.5)
resp = ser.read(ser.in_waiting).decode(errors='ignore')
test("Quick sequence (1-2-3)",
     "Do" in resp and "Re" in resp and "Mi" in resp,
     resp)

print("\n" + "="*70)
print(f"TEST RESULTS: {passed} passed, {failed} failed")
print("="*70)

ser.close()

if failed == 0:
    print("\n✓ ALL TESTS PASSED!")
    sys.exit(0)
else:
    print(f"\n✗ {failed} TEST(S) FAILED")
    sys.exit(1)
