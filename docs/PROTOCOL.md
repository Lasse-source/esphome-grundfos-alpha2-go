# GENI over BLE on the Grundfos ALPHA2 GO

Reverse-engineered from the Bluetooth traffic between the official Grundfos GO app and one ALPHA2 GO 25-75 180
(product no. 93074276), plus ESPHome's `alpha3` source for the framing. **Unofficial and incomplete.** Everything below
is an observation, not a specification. Only read requests are described.

## Bluetooth LE

| Item | Value |
|---|---|
| Advertised name | `ALPHA2 GO` |
| Service | `0xFE5D` |
| GENI characteristic | `859cffd1-036e-432a-aa28-1a0085b87ba9` |
| Value handle | `0x001a` (on the tested pump) |
| Notification descriptor (CCCD) handle | `0x001b` |
| Requests | ATT *Write Without Response* (opcode `0x52`) to `0x001a` |
| Replies | ATT *Handle Value Notification* (opcode `0x1b`) from `0x001a`, split into small pieces |
| Enable notifications | write `01 00` to `0x001b` |

Handles come from a capture of one pump; other firmware may differ.

### Security

- The phone connects and **immediately starts encryption** (`LE Start Encryption`, 0.03 s after connect) using a bond
  made earlier (LE Secure Connections, Just Works). No pairing traffic is seen in the capture.
- An **unencrypted client is disconnected about 2 s after connecting** (disconnect reason `0x13`, remote user terminated).
- Pairing a new device requires the pump's **Bluetooth button**; without it the ESP32's pairing attempt ends with
  `auth fail reason=82`.
- Order used by the app, copied by this package: **encrypt -> enable notifications -> first read -> requests**.

## Frame format (GENI)

```
SD  LE  DA  SA  APDU ...  CRC_hi CRC_lo
```

| Field | Meaning |
|---|---|
| SD | start delimiter: `0x27` request, `0x24` reply |
| LE | length: total frame size = `LE + 4` |
| DA / SA | destination / source address. Requests: DA `0xE7`, SA `0xF8` (the first read uses DA `0xFF`). Replies: DA `0xF8`, SA `0xE7` |
| CRC | CRC-CCITT (poly `0x1021`, init `0xFFFF`) over everything from `LE` up to the byte before the CRC, **bitwise inverted**, sent big-endian |

```python
def crc(frame):          # frame includes SD and CRC
    c = 0xFFFF
    for b in frame[1:-2]:
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return ~c & 0xFFFF   # must equal (frame[-2] << 8) | frame[-1]
```

A reply can arrive in several notifications; concatenate bytes and cut frames using `LE + 4`.

## Requests used

| Purpose | Frame (hex) | APDU |
|---|---|---|
| First read (the app sends it first, twice) | `27 07 ff f8 02 03 94 95 96 4f 91` | class 2 read of three values |
| Electrical block | `27 07 e7 f8 0a 03 57 00 45 8a cd` | class 10, object `57 00 45` |
| Flow / head block | `27 07 e7 f8 0a 03 5d 01 22 62 7c` | class 10, object `5d 01 22` |
| Counters | `27 07 e7 f8 0a 03 5d 00 01 45 4c` | class 10, object `5d 00 01` |

The pump answers about 80 ms after a request. The app repeats the blocks roughly once per second on its measurements screen.
ESPHome's `alpha3` component asks for flow/head with object `5d 01 21`; the ALPHA2 GO app uses `5d 01 22`.

## Replies

After the four header bytes (`SD LE DA SA`) comes the class byte `0x0a`, then **8 "type" bytes**, then the payload, then the CRC.
The header before the payload is therefore 13 bytes. Payload offsets below start after those 13 bytes. All numbers are big-endian.

| Reply | Type bytes (hex) | Payload length |
|---|---|---|
| Electrical | `30 00 01 00 03 00 00 29` | 41 |
| Flow / head | `2b 00 02 35 02 00 00 24` | 36 |
| Counters | `23 00 00 f8 02 00 00 1c` | 28 |

`alpha3` expects `2c 00 01 00 01 00 00 25` for the electrical block, which is why it discards the ALPHA2 GO's replies.

### Electrical (`57 00 45`)

| Offset | Type | Meaning |
|---|---|---|
| 0 | float | AC voltage, V |
| 4 | float | DC link voltage, V |
| 8 | float | current, A |
| 12 | float | power, W |
| 16 | float | second power-like value, close to the previous one (not identified) |
| 20 | float | motor speed, rpm |
| 24, 28 | float | `NaN` |
| 32 | float | about 51.0 in the capture (not identified) |

### Flow / head (`5d 01 22`)

| Offset | Type | Meaning |
|---|---|---|
| 0 | float | flow in m³/s, multiply by 3600 for m³/h |
| 4 | float | head, multiply by 0.0001 for m |
| 8-20 | float | `NaN` |
| 24, 28, 32 | float | three further values (about 1.5, 4.3 and 2.4 in the capture; not identified) |

### Counters (`5d 00 01`)

| Offset | Type | Meaning |
|---|---|---|
| 0 | uint32 | number of starts |
| 8 | uint32 | operating time in seconds |
| 24 | uint32 | operating time in seconds again |
| 4, 12, 16, 20 | uint32 | not identified |

## Worked example (from `tests/sample_frames.txt`)

Electrical reply: AC 235.2 V, DC link 324.0 V, current 0.169 A, power 37.88 W,
speed 4542 rpm. Flow/head reply: flow 1.53 m³/h, head 4.24 m.
Counters reply: 6 starts, operating time 31510844 s (8753 h).

## Cross-check against the GO app

The app's "all measurements" screen showed 34 W, 4360 rpm, 241 V, estimated flow 1.5 m³/h, estimated head 4.0 m, 6 starts,
and an operating time that matches the counters response. The decoded values from the capture (taken at another time)
are within a few percent of those, and voltage x current x about 0.95 matches the power value.

## Other things the app reads

The app also reads class 7 (text) objects such as the type name and product number. Those replies can contain
identifying data (including a serial number), so they are deliberately not documented or included here.
