# Electrocoagulation Water-Treatment System

This repository is an experimental water-treatment prototype that connects an ESP32-controlled tank process to a Spring Boot monitoring dashboard. The firmware fills a tank, runs a mixer and an auxiliary relay for a fixed period, drains the tank, and reports state to the backend over HTTP. The load connected to that auxiliary relay is not established by a checked-in schematic or bill of materials.

The current implementation is a development prototype. Its displayed pH and temperature are simulated by the backend, chemical-dosing outputs are not implemented, and several hardware and fail-safe details require correction before any real treatment process is attempted.

## Architecture

```text
HC-SR04 tank distance
        |
        v
      ESP32 ---- fill pump / mixer / process relay / drain pump
        |
        | HTTP POST /api/data
        v
Spring Boot backend ---- in-memory latest state ---- web dashboard
        |
        `---- GET /api/stream (includes simulated pH and temperature)
```

## Repository map

All implementation files are inside `electro_coagulation-main/`:

| Path | Purpose |
| --- | --- |
| `electro_coagulation-main/esp32_vFinal/esp32_vFinal.ino` | ESP32 firmware for level measurement, relay sequencing, Wi-Fi, and HTTP reporting. |
| `electro_coagulation-main/coag-backend/` | Spring Boot 3.5.0 backend using Java 17 and its Maven wrapper. |
| `electro_coagulation-main/coag-backend/src/main/java/tn/essat/coagbackend/controller/AppCRT.java` | `/api/data` and `/api/stream` endpoints. |
| `electro_coagulation-main/coag-backend/src/main/resources/static/index.html` | Browser dashboard. |
| `electro_coagulation-main/coag-backend/src/main/resources/application.properties` | Service binding and port configuration. |

The nested `electro_coagulation-main/README.md` is only a title; this document describes the checked-in behavior.

## Requirements

### Backend

- Java 17 or newer Java release supported by the project's Spring Boot version.
- Network access on the first build so Maven can obtain dependencies.
- Port `18000` available on the host.

The `pom.xml` includes Spring Web, Spring Data JPA, and a MySQL connector. However, `application.properties` explicitly disables DataSource auto-configuration, and the controllers keep state only in static process memory. No database is required or used by the current application.

### ESP32 firmware

- An ESP32 board and a compatible Arduino development environment.
- ESP32 Arduino-core libraries `WiFi` and `HTTPClient`.
- ArduinoJson.
- OneWire and DallasTemperature libraries are included by the sketch and must be available to compile, although the current logic does not use them.
- Proper relay/driver hardware and a safely powered HC-SR04-compatible level-sensing arrangement.

## Backend setup

In a terminal, change to the backend directory:

```text
electro_coagulation-main/coag-backend
```

On Windows:

```powershell
.\mvnw.cmd spring-boot:run
```

On Linux or macOS:

```bash
./mvnw spring-boot:run
```

The application listens on all interfaces at port `18000`. Open `http://localhost:18000/` on the backend host. From another device, use the backend computer's LAN address and allow the port through the host firewall only on a trusted development network.

The dashboard currently fetches data from a hard-coded URL:

```text
http://192.168.137.1:18000/api/stream
```

If the backend has a different address, update that URL in `electro_coagulation-main/coag-backend/src/main/resources/static/index.html` before running the application. The page also loads Chart.js and Font Awesome from public CDNs, so those visual resources require internet access unless replaced with local copies.

To run the backend tests:

```powershell
.\mvnw.cmd test
```

Use `./mvnw test` on Linux or macOS.

## ESP32 configuration and upload

Edit these placeholders near the top of `esp32_vFinal.ino`:

```cpp
const char* ssid = "Wife Name";
const char* password = "Wifi Password";
const char* serverBase = "backend API";
```

Use a base URL such as `http://192.168.1.20:18000` for `serverBase`. Keep credentials out of commits. Then:

1. Verify the pin map and the active level of every connected relay.
2. Disconnect pumps, the mixer, electrodes, and chemical equipment during the first firmware test.
3. Select the correct ESP32 board and port, compile, and upload the sketch.
4. Open the serial monitor at `115200` baud.
5. Confirm distance readings with a known physical reference.
6. Confirm each output using LEDs or other safe dummy loads.
7. Start the backend and check that `POST /api/data` returns HTTP 200 before connecting real equipment.

## Current firmware pin map

| Function | GPIO | Current behavior |
| --- | ---: | --- |
| Ultrasonic trigger | 5 | Output pulse. |
| Ultrasonic echo | 18 | Input read by `pulseIn`. Ensure the echo voltage is safe for a 3.3 V ESP32 input. |
| Fill-pump relay (`EAU`) | 13 | Control levels are internally contradictory; resolve before connection. |
| Mixer relay | 27 | Treated as active low. |
| Drain-pump relay | 26 | Treated as active low. |
| pH channel | 34 | Declared as an output in the current sketch; this is not a usable pH-sensor implementation. |
| Acid-dosing output | 12 | Initialized high/off but otherwise unused. |
| Base-dosing output | 14 | Initialized high/off but otherwise unused. |
| Auxiliary relay | 25 | Treated as active low and switched with the mixer; its connected load is not documented. |

GPIO 34 is input-only on common ESP32 devices, so `pinMode(PH, OUTPUT)` must be corrected before a pH sensor is implemented. The fill-pump logic must also be corrected: setup writes GPIO13 `LOW` with a comment saying the pump is off, but filling also uses `LOW` and stopping uses `HIGH`. If the relay is active low, the pump is energized immediately and remains so while setup blocks on Wi-Fi. If it is active high, the later fill/stop behavior is reversed. Verify all pins and safe levels against the exact ESP32 module, relay board, and wiring before connecting a pump.

## HTTP API

### Report device state

`POST /api/data` expects all four fields shown below:

```json
{
  "distance": 8.4,
  "pompe_fill": 0,
  "pompe_drain": 0,
  "mixeur": 1
}
```

Example request:

```bash
curl -X POST http://localhost:18000/api/data \
  -H "Content-Type: application/json" \
  -d '{"distance":8.4,"pompe_fill":0,"pompe_drain":0,"mixeur":1}'
```

Malformed or missing fields are not validated gracefully by the current controller.

### Read the latest dashboard state

`GET /api/stream` returns the latest posted level and actuator state plus backend-generated values:

```json
{
  "ph": 7.01,
  "temp": 25.12,
  "distance": 8.4,
  "pompe_fill": 0,
  "pompe_drain": 0,
  "mixeur": 1,
  "pompe_acide": 0,
  "pompe_base": 0
}
```

`ph` is a random walk constrained to 6.6-7.5, and `temp` is a random walk constrained to 24.3-26.5 deg C. They are simulated values, not sensor measurements. Acid/base status remains false because the backend never updates it from the current POST body.

All state is lost when the backend restarts. `@CrossOrigin("*")` permits requests from any origin.

## Process sequence in the checked-in sketch

1. Measure the ultrasonic distance.
2. If the distance is above 12 cm, command the fill output `LOW` until the reading reaches 5 cm or less, posting state while the blocking loop runs. The output is not switched `HIGH` when that loop exits.
3. If a later main-loop reading is strictly below 5 cm, command the fill output `HIGH`, run the mixer and auxiliary relay for 60 seconds, then stop them. If the reading is exactly 5 cm, neither branch runs and the prior fill output remains unchanged.
4. Run the drain pump until distance rises above 12 cm, posting state every two seconds.
5. Wait five seconds before the next main-loop iteration.

Distances from 5 through 12 cm do not start either branch. The sequence uses blocking loops and a one-minute blocking delay, so it cannot react promptly to all faults or commands.

## Known limitations

- pH and temperature are simulated in Java; there is no working pH, temperature, acid, or base feedback loop.
- The fill-pump startup comment and implemented active levels contradict each other. As written, an active-low relay would energize during setup and the blocking Wi-Fi connection, and the fill loop does not explicitly turn it off on exit.
- The pH pin is configured incorrectly, and the acid/base output variables never participate in control.
- `pulseIn(ECHO, HIGH)` has no explicit timeout. Missing echoes can produce a zero reading and drive the state machine incorrectly.
- Fill and drain loops have no independent tank-level switch, elapsed-time limit, flow confirmation, emergency stop, or relay-feedback check.
- After the drain threshold is reached, firmware switches the drain output/state off but does not POST that final state. Fill-state reporting can also remain stale until a later branch posts again.
- Wi-Fi is connected only during setup. A later disconnect causes POSTs to be skipped; there is no reconnection attempt.
- The application has no authentication or TLS. The dashboard's Logout button is visual only, and the backend accepts cross-origin requests from anywhere.
- Backend state is shared static memory, not durable storage, and concurrent/device identity handling is absent.
- The dashboard is tied to one hard-coded IP address and polls every ten seconds.
- Relay active levels and safe power-on behavior must be confirmed for the actual modules; comments and wiring documentation are incomplete.
- Water-treatment effectiveness, electrode material/spacing, current density, dose, contact time, settling, and water-quality verification are outside what the checked-in implementation establishes.

## Safety

Electrocoagulation combines electricity, water, reactive electrodes, pumps, moving equipment, and potentially acid/base chemicals. Incorrect construction or control can cause electric shock, fire, gas generation, pressure, corrosive exposure, spills, and unsafe water. Use isolated and correctly rated power systems, fusing, grounding, guarded wiring, ventilation, secondary containment, physical emergency stops, independent high/low-level switches, and qualified supervision. Never switch pumps, mixers, or electrode power directly from ESP32 pins.

Do not drink or release treated water based on the dashboard values. Use calibrated instruments and appropriate laboratory analysis, comply with local electrical/chemical/water regulations, and do not operate this prototype unattended.
