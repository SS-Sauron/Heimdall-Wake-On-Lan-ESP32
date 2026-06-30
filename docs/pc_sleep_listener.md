# Heimdall PC Sleep Listener

The `sleep_listener` is a companion application designed to run on a target PC. It securely puts the computer to sleep when it receives an authenticated command via MQTT from your Heimdall device (or any other MQTT client).

The script is fully cross-platform and works on Windows, Linux, and macOS.

## Available Versions

Inside the `experimental/scripts/` directory, you will find two versions of the listener:
1. **`sleep_listener.py`**: The raw Python script. Requires Python 3 and the `paho-mqtt` library.
2. **`sleep_listener.exe`** (inside the `dist/` folder): A standalone compiled Windows executable. It requires **no dependencies** and can be run on any Windows machine out of the box.

## Setup & Prerequisites 

If you are using the compiled `.exe` on Windows, you can skip this step.

If you are using the Python script (`.py`) on Linux, macOS, or Windows, install the required MQTT library:
```bash
pip install paho-mqtt
```

## Usage

The listener requires two mandatory arguments to function: your MQTT broker's IP address and the topic it should listen to.

```bash
# Using the Python script
python sleep_listener.py --broker 192.168.1.100 --topic wol/sleep

# Using the Windows Executable
sleep_listener.exe --broker 192.168.1.100 --topic wol/sleep
```

### All Command-Line Arguments

| Argument | Description | Default |
|---|---|---|
| `--broker` | **(Required)** IP address or hostname of your MQTT Broker. | None |
| `--topic` | **(Required)** The MQTT topic to listen on. | None |
| `--port` | The port your MQTT Broker runs on. | `1883` |
| `--user` | Username for MQTT authentication. | None |
| `--password` | Password for MQTT authentication. | None |
| `--one-shot` | If provided, the script will trigger the sleep sequence and immediately exit instead of looping forever. | Disabled |

## How to Trigger Sleep

Once the listener is running, simply publish an MQTT message containing the word `sleep` (or `suspend`, `1`, `true`, `on`, `off`) to the topic you specified. 

**Quick Test Method:** You can test the listener without a Heimdall device by connecting it to `test.mosquitto.org` and using the [HiveMQ Web Client](http://www.hivemq.com/demos/websocket-client/) (Port 8081) to publish the word `sleep` to your topic directly from your browser.

> **Warning for Windows Users:** If your computer has *Hibernation* enabled, Windows will hibernate instead of sleeping. To fix this, open an Administrator Command Prompt and run: `powercfg -h off`

## Running as a Background Service

For a permanent setup, you do not want to leave a terminal window open forever.

### Windows (NSSM)
We highly recommend using [NSSM (Non-Sucking Service Manager)](https://nssm.cc/) to run the executable silently in the background.

1. Download NSSM and open an Administrator Command Prompt.
2. Run `nssm install SleepListener`.
3. In the GUI, set the **Path** to your `sleep_listener.exe`.
4. Set the **Arguments** to `--broker 192.168.1.100 --topic wol/sleep` (include your user/password if needed).
5. Click **Install Service**, then start it with `nssm start SleepListener`.

### Linux (Systemd)
Linux requires root permissions to put the PC to sleep. Run this as a `systemd` service:

1. Create a new file at `/etc/systemd/system/sleep-listener.service`:
```ini
[Unit]
Description=Heimdall MQTT Sleep Listener
After=network.target

[Service]
Type=simple
User=root
ExecStart=/usr/bin/python3 /path/to/sleep_listener.py --broker 192.168.1.100 --topic wol/sleep
Restart=on-failure

[Install]
WantedBy=multi-user.target
```
2. Enable and start the service:
```bash
sudo systemctl daemon-reload
sudo systemctl enable sleep-listener.service
sudo systemctl start sleep-listener.service
```
