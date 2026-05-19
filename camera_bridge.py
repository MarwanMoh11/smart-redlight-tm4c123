import serial
import cv2
import re
import os

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200
OUTPUT_DIR = './captured_violations'

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

print(f"=== Laptop Webcam Bridge Active ===")
print(f"Listening on {SERIAL_PORT} at {BAUD_RATE} baud...")
print("Waiting for Tiva red light violation triggers...\n")

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Error: Could not open laptop webcam.")
        exit(1)
        
    cap.read()

    while True:
        if ser.in_waiting > 0:
            line_bytes = ser.readline()
            try:
                line = line_bytes.decode('utf-8', errors='ignore')
            except Exception:
                continue
                
            print(line, end='')

            if "[CAMERA] capture_request" in line:
                match = re.search(r'tick=(\d+)', line)
                tick = match.group(1) if match else "unknown"
                
                print(f"\n[BRIDGE ACTION] Intercepted trigger! Triggering laptop camera for tick {tick}...")
                
                for _ in range(5):
                    cap.read()
                    
                ret, frame = cap.read()
                if ret:
                    filename = os.path.join(OUTPUT_DIR, f"img_{tick}.jpg")
                    cv2.imwrite(filename, frame)
                    print(f"[BRIDGE ACTION] Snapshot successfully saved to: {filename}\n")
                else:
                    print("[BRIDGE ACTION] Error: Failed to capture image from webcam.\n")

except KeyboardInterrupt:
    print("\nTerminating Laptop Camera Bridge...")
finally:
    if 'ser' in locals() and ser.is_opened:
        ser.close()
    if 'cap' in locals():
        cap.release()
