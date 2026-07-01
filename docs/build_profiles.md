# Build Profiles and Configuration

Heimdall ships three pre-built firmware presets via the [Web Flasher](index.html). Source builders can reproduce any preset with `SDKCONFIG_DEFAULTS` fragments or customize options in `idf.py menuconfig`.

For operational behaviour and MQTT protocol details, see the [technical documentation](README.md).

---

## Web Flasher Presets

| Preset | Binary | Best for |
|--------|--------|----------|
| **Standard** | `heimdall-standard.bin` | Home networks — plug-and-play, all optional features on |
| **Hardened (Full)** | `heimdall-hardened.bin` | OPSEC deployments that still want ping, GPIO, LED, and MQTT feedback |
| **Hardened Stealth** | `heimdall-hardened-stealth.bin` | OPSEC with minimal LAN/MQTT/physical footprint |

### Standard

- Human-readable MQTT topics (`wol/<device-mac>`)
- No TOTP on wake commands
- Ping feedback, GPIO control, status LED, MQTT response channel — all enabled

### Hardened (Full)

Everything in Standard's optional feature set, plus:

- HMAC-derived opaque MQTT topics
- TOTP-signed wake commands (RFC 6238)
- MAC address spoofing and fake consumer hostname

### Hardened Stealth

Same OPSEC core as Hardened (Full), with optional features disabled:

| Disabled | Effect |
|----------|--------|
| Ping feedback | No post-wake ICMP to target PC |
| Status LED | No visual activity on GPIO2 |
| Serial provision info | Portal password not printed to USB serial |
| MQTT response channel | No wake confirmations, no `/l` heartbeats, no retained online/offline on `/s` |
| GPIO commands | No remote pin control |
| Legacy `/r` topic | No duplicate MQTT publishes |
| App + bootloader logs | Minimal UART output (`LOG_DEFAULT_LEVEL_NONE`) |

**Stealth is operational OPSEC** — reduced observability on the network and over MQTT. It is **not** ESP-IDF Secure Boot, Flash Encryption, or NVS encryption.

---

## Building from Source

Requires [ESP-IDF v6.0.1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) and `idf.py set-target esp32`.

```bash
# Standard (full-featured — default)
idf.py build

# Hardened (full)
SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.hardened" idf.py build

# Hardened Stealth
SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.hardened;sdkconfig.hardened.stealth" idf.py build
```

On Windows (PowerShell):

```powershell
$env:SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.hardened;sdkconfig.hardened.stealth"
idf.py build
```

### Configuration files

| File | Purpose |
|------|---------|
| [`sdkconfig.defaults`](../sdkconfig.defaults) | Base config; Standard profile; enables ping + GPIO |
| [`sdkconfig.hardened`](../sdkconfig.hardened) | Enables HARDENED OPSEC profile |
| [`sdkconfig.hardened.stealth`](../sdkconfig.hardened.stealth) | Stealth toggles layered on Hardened |

---

## Custom Mix (menuconfig)

After choosing a profile in **WoL Relay → Build profile**, override individual options:

| Option | menuconfig path |
|--------|-----------------|
| Build profile | **WoL Relay → Build profile** |
| MQTT response / online presence | **WoL Relay → Enable MQTT response channel** |
| Serial provisioning output | **WoL Relay → Print provisioning info to serial console** |
| Ping feedback | **Component config → Wake-on-LAN → Enable ping-based wake confirmation** |
| Status LED | **Component config → Status LED Configuration** |
| GPIO commands | **Component config → MQTT Relay → Enable GPIO output control via MQTT** |
| Legacy `/r` topic | **Component config → MQTT Relay → Also publish to legacy /r response topic** |
| HMAC topics / TOTP | **Component config → OPSEC Features** |
| MAC spoof / fake hostname | **Component config → Identity Obfuscation** |

Generate a personal defaults file from your menuconfig choices:

```bash
idf.py menuconfig
idf.py save-defconfig
```

---

## Known Stealth Limitations

These are **not** disabled in the Hardened Stealth preset today:

| Item | Notes |
|------|-------|
| **mDNS / NetBIOS** | Hostname still advertised on LAN after WiFi connect |
| **UDP WoL broadcast** | Required for core function; visible during wake |
| **Incoming ICMP to ESP** | lwIP may still respond to `ping <esp-ip>` |
| **MQTT TLS connection** | Broker still sees an active client |
| **Captive portal SoftAP** | Visible during first boot or after factory reset |

Future work may add Kconfig gates for mDNS/NetBIOS. See the [feature roadmap](heimdall_feature_roadmap.md).

---

## Manual Verification Checklist

After flashing a preset, confirm behaviour matches expectations:

### Standard

- [ ] Status LED blinks in portal mode, solid when MQTT connected
- [ ] Wake command publishes `{"status":"sent"}` on status topic (`/s`)
- [ ] Retained `{"status":"online"}` on `/s` after MQTT connect
- [ ] Ping feedback works when IP included in payload (if target allows ICMP)

### Hardened (Full)

- [ ] Same optional features as Standard
- [ ] Wake commands require valid TOTP suffix
- [ ] Opaque command topic printed once at provisioning (serial, if enabled)

### Hardened Stealth

- [ ] No status LED activity in any state
- [ ] No MQTT publish on `/s` or `/l` after wake or on connect
- [ ] No ICMP from ESP after wake (even with IP in payload)
- [ ] Minimal serial output after boot (errors may still appear on fault paths)

---

## CI Artifacts

Every push to `main` builds all three profiles. Tagged releases attach:

- `heimdall-standard.bin`
- `heimdall-hardened.bin`
- `heimdall-hardened-stealth.bin`

Shared: `bootloader.bin`, `partition-table.bin`.
