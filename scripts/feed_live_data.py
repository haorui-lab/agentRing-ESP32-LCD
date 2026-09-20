#!/usr/bin/env python3
import sys
import time
import json

PORT = "/dev/cu.usbmodem5CF71073231"

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else PORT
    print(f"Connecting to {port}...")

    payload = {
        "timestamp": int(time.time()),
        "providers": [
            {
                "id": "codex",
                "name": "Codex",
                "primary": {
                    "label": "7天额度",
                    "remainingPercent": 95.0,
                    "resetsAt": "3天 12时"
                },
                "rows": [
                    {
                        "label": "7天额度",
                        "percent": "95%",
                        "reset": "3d 12h"
                    }
                ]
            },
            {
                "id": "antigravity",
                "name": "Antigravity",
                "primary": {
                    "label": "Gemini 5小时",
                    "remainingPercent": 78.0,
                    "resetsAt": "2小时 15分"
                },
                "secondary": {
                    "label": "Gemini 7天",
                    "remainingPercent": 91.0,
                    "resetsAt": "5天 18时"
                },
                "rows": [
                    {
                        "label": "Gemini 5小时",
                        "percent": "78%",
                        "reset": "2h 15m"
                    },
                    {
                        "label": "Gemini 7天",
                        "percent": "91%",
                        "reset": "5d 18h"
                    }
                ]
            },
            {
                "id": "antigravity_third",
                "name": "Claude / GPT",
                "primary": {
                    "label": "Claude 5小时",
                    "remainingPercent": 100.0,
                    "resetsAt": "4小时 50分"
                },
                "secondary": {
                    "label": "Claude 7天",
                    "remainingPercent": 45.0,
                    "resetsAt": "6天 02时"
                },
                "rows": [
                    {
                        "label": "Claude 5小时",
                        "percent": "100%",
                        "reset": "4h 50m"
                    },
                    {
                        "label": "Claude 7天",
                        "percent": "45%",
                        "reset": "6d 02h"
                    }
                ]
            },
            {
                "id": "cursor",
                "name": "Cursor",
                "primary": {
                    "label": "Included 额度",
                    "remainingPercent": 82.0,
                    "resetsAt": "12天 06时",
                    "remainingDetails": "410 / 500"
                },
                "rows": [
                    {
                        "label": "Included 额度",
                        "percent": "82%",
                        "reset": "12d 06h"
                    }
                ]
            }
        ]
    }

    line = json.dumps(payload, ensure_ascii=False) + "\n"
    data = line.encode("utf-8")

    try:
        with open(port, "wb", buffering=0) as f:
            print("Sending live test frame...")
            f.write(data)
            f.flush()
            print("Frame sent successfully!")
            print(f"Data: {line.strip()}")
    except Exception as e:
        print(f"Error opening/writing to {port}: {e}")

if __name__ == "__main__":
    main()
