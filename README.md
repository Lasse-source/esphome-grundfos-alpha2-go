# ESPHome: Grundfos ALPHA2 GO over Bluetooth (read-only)

Read **power, voltage, current, speed, estimated flow and head, number of starts and operating time** from a
**Grundfos ALPHA2 GO** circulation pump into ESPHome / Home Assistant, using an ESP32 and the pump's Bluetooth LE.

> \*\*Status: experimental.\*\* Developed and tested against one pump (ALPHA2 GO 25-75 180, product no. 93074276) with
> ESPHome 2026.10.2 on a LilyGO T-Beam (ESP32). After pairing, values updated as expected; one 26-hour check showed
> them still updating. Not yet verified: that the pairing survives an ESP reboot, other pump firmware versions and
> other ALPHA2 GO variants. Reports welcome (see \[Contributing](#contributing)).
>
> Unofficial. Not affiliated with or endorsed by Grundfos. \*Grundfos\* and \*ALPHA2 GO\* are trademarks of their owners.

## Why this exists

ESPHome has a built-in [`alpha3`](https://esphome.io/components/sensor/alpha3/) component, but it targets the ALPHA3
(model B). On an ALPHA2 GO it connects but every value stays `nan`, for three reasons found by capturing the
official Grundfos GO app's Bluetooth traffic:

1. The pump **only talks to paired (encrypted) clients** and drops other connections about 2 seconds after connecting.
2. The ALPHA2 GO's replies have **different type bytes** than `alpha3` expects, so they are discarded.
3. Flow and head come from a **different request** (`5d 01 22` instead of `5d 01 21`).

This package fixes all three. See [docs/PROTOCOL.md](docs/PROTOCOL.md) for the details.

## What you get

|Entity|Unit|Source|
|-|-|-|
|power|W|pump|
|voltage|V|pump|
|current|A|pump|
|speed|RPM|pump|
|flow (estimated)|m³/h|pump estimate|
|head (estimated)|m|pump estimate|
|starts|count|pump|
|operating time|h|pump|
|energy (calculated)|kWh|integrated from power|

## Requirements

* An **ESP32** (needs Bluetooth; ESP8266 does not work). Tested: LilyGO T-Beam. Keep it within a few metres of the pump.
* ESPHome with the `ble\_client`, `esp32\_ble\_tracker` and `integration` components (all standard).
* Physical access to the pump's **Bluetooth button** for the first pairing.

## Quick start

1. Copy `example/grundfos-example.yaml` and `example/secrets.example.yaml` (rename the latter to `secrets.yaml`) and fill in
your WiFi details.
2. **Find the pump's Bluetooth address.** Either use a phone app such as *nRF Connect* (the device is called
`ALPHA2 GO`), or flash the example with an arbitrary `pump\_mac`, set `logger: logs: esp32\_ble\_tracker: DEBUG`
and look for `Found device ... Name: 'ALPHA2 GO'`.
3. Put the address in `pump\_mac` and flash the ESP32.
4. **Pair.** While the ESP32 is trying to connect, **press the Bluetooth button on the pump** (the wireless symbol).
Watch the log:

   * `auth fail reason=82` repeated: the pump is not in pairing mode. Press the button again.
   * `auth complete` **without** `auth fail` right after `step 0: encryption requested`: paired. The bond is stored
in the ESP32's flash, so you normally only do this once.
5. Values appear in Home Assistant a few seconds later.

Close the Grundfos GO app on your phone while testing: the pump accepts one connection at a time.

## Use as a package

```yaml
substitutions:
  pump\_mac: "AA:BB:CC:DD:EE:FF"
  pump\_name: "Grundfos ALPHA2 GO"     # optional, prefix for the entity names

packages:
  grundfos: github://Lasse-source/esphome-grundfos-alpha2-go/packages/grundfos\_alpha2\_go.yaml@main
```

Use a normal `esp32:` section with `framework: type: arduino` (as in the example).

## How it works (short)

* GENI frames are sent as BLE *write without response* to one characteristic and answered as notifications.
* A small parser reassembles the notifications into frames, checks the CRC, recognises the reply type and publishes the values.
* On connect the ESP32 follows the same order as the GO app: **encryption, enable notifications, first read, then requests**.
* Only **read requests** are sent. Nothing in this package writes pump settings.

The parser can be tested on a PC without hardware: `sh tests/run\_tests.sh` (needs `g++`).

## Limitations and caveats

* The GATT handles `0x001a` (data) and `0x001b` (notifications) were read from captured traffic. ESPHome releases the
service list after connecting, so they are hard-coded and could differ on other firmware.
* Flow and head are the pump's own **estimates**.
* The pump's own cumulative kWh counter was not found in the captures, so energy is integrated from power.
* Several fields in the replies are not identified (see PROTOCOL.md).
* Unknown whether the pump keeps the phone's bond when another device is paired. If the GO app stops connecting after
you paired the ESP32, pair the phone again.
* If you erase the ESP32's flash completely, pair again.

## Contributing

Reports from other pumps and firmware versions are very welcome: open an issue with your pump model, product number
and what worked. If you need to capture traffic, **read** [**docs/CAPTURING.md**](docs/CAPTURING.md) **first**: a raw Bluetooth
capture contains your pairing keys and must not be posted. Use `tools/extract\_geni\_frames.py` to share only the pump frames.

## Credits

Builds on the ideas of ESPHome's `alpha3` component (GENI over BLE for the ALPHA3). The protocol details for the
ALPHA2 GO were worked out from the author's own pump and phone traffic for the purpose of interoperability.

## License

MIT, see [LICENSE](LICENSE).

