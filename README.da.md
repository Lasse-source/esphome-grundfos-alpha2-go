# ESPHome: Grundfos ALPHA2 GO over Bluetooth (kun læsning)

Aflæs **effekt, spænding, strøm, omdrejninger, estimeret flow og løftehøjde, antal starter og driftstid** fra en
**Grundfos ALPHA2 GO** cirkulationspumpe i ESPHome / Home Assistant med en ESP32.

Status: eksperimentelt, testet på én pumpe (ALPHA2 GO 25-75 180). Uofficielt og uden tilknytning til Grundfos.

**Kort fortalt:**

1. Pumpen taler kun med **parrede, krypterede** forbindelser og lukker andre efter ca. 2 sekunder.
2. Svarene har **andre typebytes** end ESPHome's `alpha3`, så de bliver kasseret.
3. Flow og løftehøjde hentes med en **anden forespørgsel** (`5d 01 22`).

Sådan kommer du i gang:

1. Kopiér `example/grundfos-example.yaml`, og udfyld WiFi i `secrets.yaml` (se `example/secrets.example.yaml`).
2. Find pumpens Bluetooth-adresse (nRF Connect, enheden hedder `ALPHA2 GO`), og sæt den i `pump_mac`.
3. Flash ESP32'en.
4. **Tryk på pumpens Bluetooth-knap**, mens ESP'en forsøger at forbinde. Se efter `auth complete` uden `auth fail` i loggen.
5. Luk Grundfos GO-appen under testen, for pumpen tager kun én forbindelse ad gangen.

Mere i [README.md](README.md) (engelsk) og [docs/PROTOCOL.md](docs/PROTOCOL.md). Del aldrig en rå Bluetooth-optagelse:
den indeholder dine parringsnøgler. Brug `tools/extract_geni_frames.py`.
