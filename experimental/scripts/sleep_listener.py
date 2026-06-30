#!/usr/bin/env python3
import os
import platform
import subprocess
import argparse
import logging
import sys

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("Error: paho-mqtt library is not installed.")
    print("Please install it using: pip install paho-mqtt")
    sys.exit(1)

logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')

def put_pc_to_sleep():
    """
    Cross-platform function to put the computer to sleep.
    """
    os_name = platform.system().lower()
    
    try:
        if os_name == 'windows':
            logging.info("Triggering sleep on Windows...")
            import ctypes
            # SetSuspendState(hibernate, forceCritical, disableWakeEvent)
            # 0, 1, 0 means: don't hibernate (sleep), force immediate, don't disable wake events
            # NOTE: If hibernation is enabled in Windows, this will hibernate instead of sleep.
            # You can disable hibernation by running 'powercfg -h off' as administrator.
            result = ctypes.windll.powrprof.SetSuspendState(0, 1, 0)
            if not result:
                logging.error("SetSuspendState failed.")
                
        elif os_name == 'linux':
            logging.info("Triggering sleep on Linux...")
            # For modern Linux with systemd
            subprocess.run(["systemctl", "suspend"], check=True)
            
        elif os_name == 'darwin':
            logging.info("Triggering sleep on macOS...")
            subprocess.run(["pmset", "sleepnow"], check=True)
            
        else:
            logging.error(f"Unsupported operating system: {os_name}")
            
    except subprocess.CalledProcessError as e:
        logging.error(f"Command failed (do you have sufficient privileges?): {e}")
    except Exception as e:
        logging.error(f"Failed to execute sleep command: {e}")


def on_connect(*args, **kwargs):
    """
    Flexible on_connect signature to handle both paho-mqtt v1 and v2.
    v1: on_connect(client, userdata, flags, rc)
    v2: on_connect(client, userdata, flags, reason_code, properties)
    """
    client = args[0]
    userdata = args[1]
    rc = args[3] # reason_code or rc is the 4th argument in both versions
    
    if rc == 0:
        logging.info(f"Connected to MQTT broker. Subscribing to topic: {userdata['topic']}")
        client.subscribe(userdata['topic'])
    else:
        logging.error(f"Failed to connect to MQTT broker. Return code: {rc}")


def on_message(*args, **kwargs):
    """
    Flexible on_message signature to handle both paho-mqtt v1 and v2.
    v1: on_message(client, userdata, message)
    """
    msg = args[2]
    payload = msg.payload.decode('utf-8', errors='ignore').strip()
    logging.info(f"Received message on {msg.topic}: {payload}")
    
    if payload.lower() in ["sleep", "suspend", "1", "true", "on", "off"]:
        logging.info("Valid sleep command received. Executing sleep sequence...")
        put_pc_to_sleep()
        if args[1].get("one_shot"):
            logging.info("One-shot mode enabled. Exiting script.")
            args[0].disconnect()
    else:
        logging.info(f"Ignored message. Payload '{payload}' did not match known sleep commands.")


def main():
    parser = argparse.ArgumentParser(description="Cross-platform MQTT Sleep Listener")
    parser.add_argument("--broker", required=True, help="MQTT Broker address (e.g., 192.168.1.100 or test.mosquitto.org)")
    parser.add_argument("--port", type=int, default=1883, help="MQTT Broker port (default 1883)")
    parser.add_argument("--topic", required=True, help="MQTT Topic to listen on (e.g., wol/sleep)")
    parser.add_argument("--user", help="MQTT Username (optional)")
    parser.add_argument("--password", help="MQTT Password (optional)")
    parser.add_argument("--one-shot", action="store_true", help="Exit the script immediately after triggering sleep once")
    
    args = parser.parse_args()
    
    # Handle paho-mqtt v2 CallbackAPIVersion requirement if it exists
    if hasattr(mqtt, "CallbackAPIVersion"):
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, userdata={"topic": args.topic, "one_shot": args.one_shot})
    else:
        client = mqtt.Client(userdata={"topic": args.topic, "one_shot": args.one_shot})
    
    if args.user and args.password:
        client.username_pw_set(args.user, args.password)
        
    client.on_connect = on_connect
    client.on_message = on_message
    
    logging.info(f"Connecting to broker {args.broker}:{args.port}...")
    try:
        client.connect(args.broker, args.port, 60)
    except Exception as e:
        logging.error(f"Connection failed: {e}")
        sys.exit(1)
        
    try:
        logging.info("Starting listener loop. Press Ctrl+C to exit.")
        client.loop_forever()
    except KeyboardInterrupt:
        logging.info("Shutting down listener...")
        client.disconnect()

if __name__ == "__main__":
    main()
