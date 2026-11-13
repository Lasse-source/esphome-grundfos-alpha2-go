# Capturing Bluetooth traffic safely

You only need this if the package does not work on your pump and you want to help work out why.

## Privacy warning

A raw Bluetooth HCI capture (`btsnoop_hci.log`) contains:

- the **pairing key (LTK)** of your bonded devices (visible in the `LE Start Encryption` command), which would let someone
  talk to your pump as your phone,
- advertisements and addresses of **every Bluetooth device nearby**,
- the pump's address and measurement data.

**Never attach, upload or paste a raw capture or a bug report.** Share only the extracted GENI frames
(see below), after checking them.

## Capture on Android

1. Developer options -> **Bluetooth HCI snoop log** -> choose the **full** option (not "filtered" if both exist). Toggle Bluetooth off and on.
2. Stand within two metres of the pump. Open the Grundfos GO app, connect, and open the screen with all measurements.
   Leave it for about 30 seconds, then close the app.
3. Take a bug report (Developer options -> Take bug report). In the zip, the full capture is usually at
   `FS/data/misc/bluetooth/logs/btsnoop_hci.log`. The `btsnooz` text in the report is a shortened summary and not enough.
4. Turn the snoop log off again and delete the capture when you are done.

## Extract only the pump frames

```
python3 tools/extract_geni_frames.py btsnoop_hci.log --mac AA:BB:CC:DD:EE:FF > frames.txt
```

The output has one frame per line, `S` for phone to pump and `R` for pump to phone, and nothing else. The script needs
only Python 3. Look through `frames.txt` before sharing: replies to **class 7** requests (frames starting `24 .. f8 e7 07`)
contain text such as the type name, product number and **serial number**. Delete those lines.

You can analyse the frames with the checks in `docs/PROTOCOL.md`: verify the CRC, group replies by their 8 type bytes,
and compare decoded numbers with what the GO app shows.
