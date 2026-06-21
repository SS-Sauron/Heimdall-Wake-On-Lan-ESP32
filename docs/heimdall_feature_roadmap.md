# Heimdall Feature Roadmap & Deep Dive

Based on the analysis of 11 alternative ESP32 Wake-on-LAN projects, here is the comprehensive list of features we can integrate into Heimdall. Each feature is broken down by **Why** it adds value, **How** we would implement it technically, and **What could cause problems** with the current architecture.

---

## 1. Named Target Profiles / Multi-Device Shortcuts

**Why to add it:**
Heimdall can already wake any Wake-on-LAN-capable device on the same LAN broadcast domain by publishing that device's MAC address in the MQTT payload. It does not store a single target MAC in NVS, and users do not need a separate ESP32 per computer.

The useful feature here is not basic multi-MAC support; it is convenience: named saved targets, shortcuts, and less error-prone commands for users who wake several machines regularly.

**How to add it:**
- **Keep existing raw-MAC payloads:** Preserve today's behavior where payloads such as `AA:BB:CC:DD:EE:FF` and, in hardened builds, `AA:BB:CC:DD:EE:FF:123456` wake whichever target MAC is provided.
- **Optional NVS registry:** Add a separate saved-target list, for example `[{ "label": "desktop", "mac": "AA:BB...", "ip": "192.168.1.50" }]`. The IP should be optional and used only by future reachability checks.
- **Portal UI:** Add a compact target-profile editor only for convenience. It should not be required for basic operation.
- **MQTT Routing:** Continue supporting the existing command topic and raw MAC payload. Optionally add JSON payloads such as `{"target":"desktop"}` or topic shortcuts such as `<command_topic>/desktop`, but treat these as aliases that resolve to a saved MAC.

**What could cause a problem (ESP-IDF Verified):**
- **NVS Blob Limits:** Verified against ESP-IDF docs. NVS blobs are limited to 508,000 bytes, and strings to 4000 bytes. We will not hit this limit.
- **RAM Usage:** The Captive Portal is tight on heap. Generating a dynamic HTML page for multiple MACs or parsing a larger JSON payload could push us into Out-of-Memory (OOM) territory if we aren't careful with `cJSON`.
- **Compatibility:** Existing raw-MAC payloads must remain valid so current trigger scripts and Home Assistant automations do not break.

---

## 2. Post-WOL Reachability Confirmation (Ping / TCP Probe)

**Why to add it:**
Right now, Heimdall fires the Wake-on-LAN magic packet and immediately publishes `{"status":"sent"}`. It has no idea if the PC actually woke up. Competitors use smart plugs to verify power draw or ICMP ping. We can do this entirely in software.

**How to add it:**
- **Firmware task:** After sending the WOL packet, spawn a lightweight FreeRTOS task.
- **Probing:** The task attempts to connect to the target machine's IP address on a specific TCP port (e.g., 22 for SSH, 3389 for RDP) or uses raw sockets for an ICMP Ping.
- **Reporting:** It polls every 5 seconds for up to 2 minutes. When it gets a connection (or times out), it publishes to the MQTT response topic: `{"status":"online", "latency_ms": 45}`.

**What could cause a problem (ESP-IDF Verified):**
- **LwIP Configuration:** Verified against ESP-IDF docs. We must ensure `CONFIG_LWIP_ICMP=y` is set in `sdkconfig` to use `ping/ping_sock.h`.
- **Target IP Requirement:** WOL only requires a MAC address. A Ping probe requires knowing the target PC's static IP. We would need to ask the user to input the IP address in the Captive Portal.
- **Firewall Interference:** Windows Firewall often blocks ICMP Ping requests by default. It could result in false negatives (`{"status":"timeout"}`) even when the PC woke up successfully, confusing the user.

---

## 3. In-Place Configuration via MQTT

**Why to add it:**
If a user changes network details such as the broker hostname, MQTT credentials, device hostname, or a future saved target profile, they currently need to use the captive portal flow again. However, replacing a PC motherboard or NIC does not require a factory reset today: the user can already publish the new target MAC in the command payload.

**How to add it:**
- **New MQTT Topic:** Subscribe to a configuration topic like `wol/config`.
- **Authenticated Payload:** Expect a JSON payload authenticated by the same hardened-mode security model, for example `{"action":"set_hostname","hostname":"relay-office"}` or, after named targets exist, `{"action":"upsert_target","label":"desktop","mac":"11:22:33:44:55:66"}`.
- **Execution:** Validate the requested change, write the relevant NVS key through the storage component, and publish an acknowledgment. For WiFi or MQTT credential changes, prefer a staged update plus explicit reboot because a typo can disconnect the relay from the broker.

**What could cause a problem (ESP-IDF Verified):**
- **NVS Thread Safety:** Verified against ESP-IDF docs. Writing to NVS (`nvs_set_blob`, `nvs_commit`) from the MQTT task while other tasks are running is explicitly thread-safe in ESP-IDF.
- **Security Scope Creep:** Allowing remote writes to NVS opens a tiny attack surface. If a bug exists in our JSON parser, a remote command could corrupt the NVS partition or crash the device.
- **Loss of Connectivity:** If the user updates MQTT or WiFi settings incorrectly, the device may lose contact and require a factory reset. Target MAC typos are less dangerous because raw-MAC payloads can still be sent directly.

---

## 4. Browser-Based Web Trigger UI (Static App)

**Why to add it:**
Right now, the user has to write a Python script, configure Home Assistant, or use an MQTT app on their phone to trigger Heimdall. A custom, static HTML page hosted on GitHub Pages would allow them to hit a "Wake PC" button from any browser in the world.

**How to add it:**
- **Frontend Code:** Write a single `index.html` file using standard Javascript and the `mqtt.js` library (using WebSockets to talk to the MQTT broker).
- **Local Storage:** The page prompts for the user's MQTT broker, Username, Password, and HMAC Secret. It saves these purely in the browser's `localStorage` for future visits.
- **Hosting:** Host it for free on GitHub Pages. Users just bookmark `yourusername.github.io/heimdall-ui`.

**What could cause a problem:**
- **Broker WebSocket Support:** Browsers cannot make raw TCP MQTT connections; they require the broker to support MQTT over WebSockets (usually port 8083 or 8084). If the user's broker (e.g., HiveMQ free tier or a basic local Mosquitto setup) doesn't have WebSockets enabled, the web UI won't be able to connect.

---

## 5. WOL UDP Relay/Bridge Mode

**Why to add it:**
Heimdall currently *only* listens to MQTT. If a user is on their local network and uses a standard phone app like "Wake On Lan", Heimdall ignores it. By acting as a UDP bridge, Heimdall can listen for standard WOL packets coming from a VPN or external subnet, and re-broadcast them to the local subnet.

**How to add it:**
- **UDP Listener Task:** Create a task that binds to UDP port 9.
- **Packet Validation:** When a packet is received, verify it matches the 102-byte Magic Packet format.
- **Re-broadcast:** Call `wol_send_raw()` to blast it out to the local subnet, ensuring it reaches sleeping PCs that the original packet might not have reached due to router broadcast rules.

**What could cause a problem (ESP-IDF Verified):**
- **LwIP TX Buffers:** Verified against ESP-IDF docs. Calling `sendto()` repeatedly can return `ENOMEM` if driver transmit buffers are full. Since we only send occasional 102-byte packets, this is safe.
- **Infinite Loops (Broadcast Storms):** If Heimdall broadcasts a packet, and its own UDP listener hears that broadcast, it might re-broadcast it indefinitely, crippling the local network. We must filter out packets originating from the ESP32's own IP address.

---

## 6. Browser-Based Firmware Flashing (ESP Web Tools)

**Why to add it:**
Currently, installing Heimdall requires setting up the ESP-IDF toolchain or running a script. Providing a webpage that flashes the ESP32 directly via USB (WebSerial) allows non-developers to install Heimdall in one click.

**How to add it:**
- **Manifest:** Create a `manifest.json` pointing to the pre-compiled `.bin` files (bootloader, partition table, firmware).
- **Webpage:** Add `<esp-web-install>` tags to the repository's GitHub Pages or README. Users plug in the ESP32 via USB, click "Install", and the browser flashes it.

**What could cause a problem (ESP-IDF Verified):**
- **Partition Alignment:** Verified against ESP-IDF docs. ESP Web Tools flashes raw binaries (`.bin`). It requires the `partitions.bin` and `bootloader.bin` to be provided and correctly aligned (e.g. `0x8000` and `0x1000`) in the `manifest.json`.
- **Browser Compatibility:** WebSerial is only supported on Chromium-based browsers (Chrome, Edge, Brave). Firefox and Safari users will be unable to use it.
- **Driver Issues:** Windows users sometimes still need to download CH340 or CP2102 serial drivers to see the COM port in the browser prompt.
- **Release Automation:** We would need to set up GitHub Actions to automatically compile the `.bin` files and update the manifest on every commit.

---

## 7. SecureOn (6-Byte WOL Password) Support

**Why to add it:**
Some high-end or enterprise motherboards require a "SecureOn" password appended to the magic packet. If it's missing, the PC ignores the WOL request.

**How to add it:**
- **NVS Field:** Add a 6-byte field to `storage_credentials_t`.
- **Portal UI:** Add an optional "SecureOn Password" input field in the Captive Portal.
- **Packet Construction:** Modify `wol.c` to append the 6 bytes to the end of the 102-byte magic packet buffer before sending.

**What could cause a problem:**
- **Niche Clutter:** 99% of consumer motherboards don't use this. Adding it to the portal might confuse standard users who wonder if they need to fill it out.

---

## 8. Status LED Feedback

**Why to add it:**
Heimdall operates invisibly. If it loses WiFi, the user has no idea unless they connect a serial monitor or check their MQTT broker.

**How to add it:**
- **Hardware Profile:** Map an internal GPIO (often GPIO 2) or let the user define a custom LED pin in `menuconfig`.
- **Blink Patterns:**
  - Fast blink: Portal active / AP mode.
  - Slow pulse: Connecting to WiFi / MQTT.
  - Solid: Connected and ready.
  - Flash: WOL packet dispatched.

**What could cause a problem:**
- **Board Variations:** Not all ESP32 boards have built-in LEDs, and those that do use different GPIOs (GPIO 2, GPIO 33, etc.). If a user selects the wrong pin, it might interfere with flash memory lines or bootstrap pins.

---

## 9. Setup/Installation Scripts

**Why to add it:**
Competitors like `medinajaime/esp32-wakeonlan` provide `setup.ps1` and `setup.sh` that automatically download the ESP-IDF toolchain, build the project, and flash it without the user needing to type `idf.py` commands manually.

**How to add it:**
- **Scripts Directory:** Write shell and PowerShell scripts that check if ESP-IDF is in the environment, download it if not, run `idf.py menuconfig`, and then build/flash.

**What could cause a problem:**
- **Maintenance Burden:** Supporting scripts across Windows, WSL, macOS, and different Linux distros is notoriously painful. Paths break, Python environments conflict, and users open GitHub issues because `setup.sh` failed due to their OS configuration.

---

### Conclusion & Next Steps

If we want to transition Heimdall from an "excellent prototype" to a more polished tool, the most useful near-term additions are:

1. **Status LED Feedback** (Lowest-risk quality-of-life improvement)
2. **Post-WOL Reachability** (Closes the feedback loop for the user)
3. **Named Target Profiles** (Convenience layer for multi-device households, while preserving existing raw-MAC payloads)

Let me know which features you want to prioritize, and I can begin drafting the architectural plans for them.
