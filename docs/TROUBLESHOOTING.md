# Troubleshooting

| Symptom in the ESPHome log | Likely cause | What to do |
|---|---|---|
| `auth fail reason=82` on every connection | ESP32 is not paired and the pump is not in pairing mode | Press the pump's Bluetooth button while the ESP32 tries to connect, until you see `auth complete` **without** `auth fail` |
| Connect, disconnect about 2 s later, repeat; no `geni_raw` lines | Link is not encrypted/paired (the pump drops it) | Same as above. Check that `step 0: encryption requested` appears in the log |
| `step ... sent` appears, `geni_raw` appears, values stay `nan` | Reply type differs on your model/firmware | Set `geni_raw` to `DEBUG`, open an issue with the **pump model, product number** and a few `geni_raw` lines |
| `bad checksum, frame dropped` | Corrupt or partial frame | Occasional ones are harmless. Many: move the ESP32 closer, check WiFi/BLE coexistence |
| `Disconnected!` shortly after pairing worked | Phone app connected to the pump, or weak signal | Close the Grundfos GO app, put the ESP32 within a few metres |
| GO app can no longer connect | The pump may remember a limited number of bonded devices | Pair the phone again with the pump's button |
| Values stop after weeks | Bond lost (flash erased) or pump firmware changed handles | Pair again; if it still fails, open an issue |

## Reading the log

- `step 0: encryption requested (status 0)`: the ESP32 asked for an encrypted link.
- `auth complete` right after it, with no `auth fail`: paired.
- `step 2: notifications enabled`: the notification descriptor was written again after encryption.
- `step 3..9 sent`: reads were sent. The first replies should appear within a second.

## Before you post logs

Logs contain your pump's Bluetooth address, your WiFi name and IP addresses. Remove them before posting.
