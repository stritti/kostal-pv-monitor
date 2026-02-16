# AGENTS.md

Zweck: Regeln und Standards für einen Coding-Agent, der PlatformIO-basierten IoT-Firmware für ESP32 entwickelt, refaktoriert, analysiert und stabilisiert. Fokus: Architektur, Linting, Speicher- und Ressourcenmanagement, Sicherheit, 24/7-Betrieb, Wartbarkeit, Tests und OTA-Updates für das Kostal Plenticore PV-Monitoring-System.

## 1. Arbeitsweise

Der Agent:

- liefert minimal-invasive Änderungen mit Begründung, Impact-Analyse (RAM/Flash/CPU), Risiken, Rollback-Hinweisen.
- verhindert blockierende Patterns im Laufzeit-Code (long `delay()`, busy waits, blockierende Netzwerk-Calls im Main-Loop).
- vermeidet unsichere dynamische Allokationen in Hot-Paths und ungeprüfte Heap-Nutzung.
- folgt einem CI-regelbasierten Prozess (Build + Tests + Lint).
- berücksichtigt Deep-Sleep-Zyklen und Power-Management für batteriebetriebene E-Paper-Displays.

## 2. Zielplattform & Framework

- Hardware: ESP32 (esp32dev) mit TTGO T5 E-Paper Display
- Framework: Arduino auf ESP32
- PlatformIO: Single-Source für Build-Konfiguration (`platformio.ini`), Projekt-Environments, Lib-Pins
- Display: E-Paper mit Low-Power-Eigenschaften, Update-Zyklen optimiert

## 3. Projektstruktur und Architektur

Aktuelle Struktur:

- `src/main.cpp` Hauptanwendungslogik mit Deep-Sleep-Management
- `src/kostal_modbus.h` Modbus-Kommunikation mit Kostal Plenticore Inverter
- `src/ntp_localtime.h` Zeitverwaltung und NTP-Synchronisation
- `src/u8g2_display.h` E-Paper Display-Treiber und -Verwaltung
- `src/board_def.h` Board-spezifische Pin-Definitionen
- `docs/` VitePress-Dokumentation

Architekturregeln:

- Modbus-Kommunikation erfolgt in kontrollierten Bursts mit Rate-Limiting (5s zwischen Batches, 100ms zwischen Queries)
- Display-Updates nur während Tageszeit (7-23 Uhr) zur Stromersparnis
- Deep-Sleep zwischen Messwerten zur Batterieschonung
- Fehler-Resilienz durch Timeout-Mechanismen und Retry-Logic
- Keine blockierenden Operationen im Main-Loop

## 4. Coding-Standards

- Sprache: C++11 (per platformio.ini)
- Header: `include-what-you-use`, kein globales `using namespace`
- Konstanten: Named constants statt Magic Numbers
- Buffer-Sicherheit: `snprintf()` statt `sprintf()` zur Overflow-Prävention
- Variable Initialisierung: Alle Variablen vor Verwendung initialisieren
- Input-Validierung: Hostname, IP-Adressen validieren vor Verwendung
- Fehlerbehandlung immer explizit, kein stilles Ignorieren
- Logging: Strukturiert mit Loglevel-Kontrolle via `CORE_DEBUG_LEVEL`
- Timeout-Schutz: Alle Netzwerk-Operationen (WiFi, NTP, Modbus) mit konfigurierbaren Timeouts

## 5. Linting & Format

- Format: `.clang-format` und `.editorconfig` im Repository
- Statische Analyse: `cppcheck` mit high/medium severity
- Check-Flags: `--std=c++11 --enable=warning,style,performance,portability`
- CI: PlatformIO CI-Workflow (`pio run`, `pio check`)
- Keine neuen Warnungen akzeptieren

## 6. Commit-Konventionen (Conventional Commits)

Commit Messages müssen dem Conventional Commits-Standard folgen: `<type>[optional scope]: <description>`

Commit Types:

- `feat` für neue Funktionen
- `fix` für Fehlerbehebungen
- `docs` für Änderungen an Dokumentation
- `style` für Formatierung/Code Style
- `refactor` für Code-Umstrukturierungen ohne funktionale Änderung
- `perf` für Performance-Optimierungen
- `test` für Test-Änderungen
- `chore` für Wartung/Tooling/Build-Änderungen

Breaking Changes müssen mit `BREAKING CHANGE:` im Footer markiert werden.

## 7. Build-Konfiguration

- `platformio.ini`: zentrale Flags, zwei Environments
  - **ttygo-t5** (default): Release-Build mit `-Os` (Size-Optimierung), `CORE_DEBUG_LEVEL=0`
  - **ttygo-t5-debug**: Debug-Build mit `CORE_DEBUG_LEVEL=3` (Verbose Logging)
- Serial: 115200 Baud
- Upload-Speed: 2000000 Baud
- Monitor-Filter: `esp32_exception_decoder, time, default`

## 8. Speicher & Ressourcen

- Kein unbounded dynamic Heap/Fragmentierung:
  - Statische Puffer wo möglich (`snprintf` mit festen Buffer-Größen)
  - String-Objekte in Loops vermeiden
- E-Paper Display: Framebuffer-Speicher effizient nutzen
- Deep-Sleep reduziert RAM-Anforderungen zwischen Updates
- Heap-Monitoring: `ESP.getFreeHeap()` nutzen zur Laufzeitüberwachung
- Buffer-Overflow-Schutz: Alle String-Operationen mit Bounds-Checking

## 9. Kostal Modbus Best Practices

- **Rate Limiting**: Minimum 5 Sekunden zwischen Modbus-Query-Batches
- **Inter-Query-Delay**: 100ms zwischen einzelnen Register-Reads
- **Connection-Management**: Stabile Verbindung während Reads, dann graceful close
- **Retry-Logic**: Exponential Backoff (500ms → 1s → 2s)
- **Transaction-Timeouts**: 5 Sekunden pro Transaktion
- **Proper Cleanup**: Connection ordnungsgemäß schließen zur Ressourcenfreigabe
- **Validation**: Remote IP und Hostname validieren vor Connection-Attempt
- **Error Recovery**: Safe default values statt Crashes bei fehlgeschlagenen Operationen

## 10. Power Management & Display

- **Deep Sleep**: Device schläft zwischen Messungen (typisch 5-15 Minuten)
- **Tageszeit-Abhängigkeit**: Display-Updates nur 7-23 Uhr
- **E-Paper Optimierung**: Partielle Updates wo möglich, Full-Refresh minimieren
- **WiFi Management**: Verbindung nur für Datenabruf, dann disconnect
- **Batterie-Monitoring**: Battery-Level-Tracking implementieren (geplant)

## 11. Netzwerk-Sicherheit

- **Timeout-Protection**: Alle Netzwerk-Operationen mit konfigurierbaren Timeouts
- **Connection-Validation**: IP und Hostname validieren
- **Error-Handling**: Comprehensive Error Messages für Debugging
- **WiFi-Credentials**: Über ESP-WiFiSettings zur Runtime-Konfiguration
- **Keine Secrets im Repository**: Credentials nicht hardcoden

## 12. OTA & Updates

- OTA-Capability mit sicherer Update-Prozedur
- Update-Failure-Detection vor Aktivierung neuer Firmware
- HTTPS für OTA-Downloads empfohlen
- Rollback-Mechanismus bei fehlgeschlagenen Updates

## 13. 24/7-Robustheit

- Watchdogs aktiv für Loop/Task-Monitoring
- Netzwerk-Resilienz: Reconnect mit Exponential Backoff
- NTP-Time mit Retry-Logic und Fallback
- Persistenz: Minimierte Flash-Writes
- Health-Metrics: Uptime, Heap-Status, WiFi-Status
- Reset-Reason-Tracking zur Fehleranalyse

## 14. Logging & Telemetrie

- Strukturierte Logs mit Loglevel-Kontrolle
- Debug-Build: Verbose Logging (`CORE_DEBUG_LEVEL=3`)
- Release-Build: Minimal Logging (`CORE_DEBUG_LEVEL=0`)
- Rate Limits für wiederkehrende Events
- Exception-Decoder für Crash-Analyse

## 15. Konfiguration & Secrets

- WiFi-Settings zur Runtime über ESP-WiFiSettings-Bibliothek
- Modbus-Konfiguration (Hostname/IP) validiert vor Verwendung
- Keine hartkodierten Credentials
- Board-spezifische Definitionen in `board_def.h`

## 16. Tests

- Test-Infrastruktur vorhanden (`test/` Verzeichnis)
- Desktop-Tests ignoriert (`test_ignore = test_desktop`)
- Unit-Tests für Modbus-Parser, NTP-Logic, Display-Rendering
- Native Tests für CI bevorzugt

## 17. Dependencies

Verwendete Libraries (Version-Pinned in platformio.ini):

- `adafruit/Adafruit BusIO@^1.11.3`
- `adafruit/Adafruit GFX Library@^1.10.12`
- `zinggjm/GxEPD@^3.1.1` (E-Paper Display)
- `emelianov/modbus-esp8266@^4.1.0` (Modbus TCP)
- `juerd/ESP-WiFiSettings@^3.8.0` (WiFi-Konfiguration)
- `me-no-dev/AsyncTCP@^1.1.1`
- `olikraus/U8g2@^2.32.10`
- `olikraus/U8g2_for_Adafruit_GFX@^1.8.0`
- `arduino-libraries/NTPClient@^3.2.1`
- `madpilot/mDNSResolver@^0.3`
- `jchristensen/Timezone@^1.2.4`
- `Wire@2.0.0`

Dependency-Management:

- Minimiert, begründet, Version-Pinned
- Lizenz-Checks (alle unter MIT/Apache/BSD)
- Updates mit CI-Absicherung

## 18. Release & CI

- **GitHub Actions**: PlatformIO CI Workflow
- **Release-Build**: Sicherheitsfeatures aktiv, Debug-Output deaktiviert
- **CI-Checks**: Build, Lint (cppcheck), Format-Checks
- **Size-Optimierung**: `-Os` Flag für Release
- **Badges**: CI-Status und License-Badge im README

## 19. Antipatterns (verboten)

- Unlimitierte `delay()` im Main-Loop
- Busy-Wait-Schleifen
- Blockierende Netzwerk-Calls ohne Timeout
- Häufige Heap-Allokationen in Hot-Paths
- `sprintf()` statt `snprintf()` (Buffer-Overflow-Risiko)
- String-Objekte in zyklischem Code
- Unvalidierte User-Inputs (Hostname, IP)
- Globaler State ohne Synchronisation
- Fehlende Initialisierung von Variablen

## 20. Projektspezifische Besonderheiten

- **Modbus-Protokoll**: Kostal Plenticore Modbus TCP SunSpec Hybrid (siehe `docs/BA_KOSTAL-Interface-description-MODBUS-TCP_SunSpec_Hybrid.pdf`)
- **Display-Feedback**: Smiley-Anzeige basierend auf primärer Energiequelle:
  - Batterie: 🙂
  - PV: 😎
  - Grid: 🙁
- **Metriken**: PV-Produktion, Batterie-Lade/Entlade-Status & SoC, Hausverbrauch, Netz-Bezug/Einspeisung
- **E-Paper-Spezifika**: Partial-Updates für schnelle Aktualisierungen, Full-Refresh alle N Updates
- **LiPo-Batterie**: Geplante Integration mit Ladecontroller und Battery-Level-Monitoring

## 21. Dokumentation

- **VitePress**: Dokumentation in `docs/` mit VitePress ^1.5.0
- **README.md**: Projekt-Übersicht, Build-Instructions, Security Best Practices
- **Inline-Docs**: Funktionen mit detaillierten Kommentaren dokumentieren
- **Architecture-Decisions**: Wichtige Design-Entscheidungen dokumentieren

## 22. Millis() Overflow Handling

- **Kritisch**: Alle `millis()`-basierten Timeouts und Rate-Limiting müssen overflow-safe sein
- **Pattern für Timeouts**: `(millis() - startTime) >= timeout`
- **Pattern für Rate-Limiting**: `(millis() - lastTime) < delay`
- **Begründung**: Nach ~49 Tagen läuft `millis()` über, unsichere Vergleiche führen zu Fehlfunktionen
- **Locations**: Siehe Repository Memories für bekannte Locations

## Zusammenfassung

Dieser Agent entwickelt IoT-Firmware für ein ESP32-basiertes Kostal-PV-Monitoring-System mit E-Paper-Display. Der Fokus liegt auf:

1. **Sicherheit**: Buffer-Overflow-Schutz, Input-Validierung, Timeout-Management
2. **Zuverlässigkeit**: Retry-Logic, Error-Handling, Watchdogs
3. **Effizienz**: Deep-Sleep, Power-Management, Rate-Limiting
4. **Wartbarkeit**: Strukturierter Code, Conventional Commits, CI/CD
5. **Best Practices**: Modbus-Protokoll korrekt implementiert, E-Paper optimal genutzt

Alle Änderungen müssen minimal-invasiv sein, dokumentiert und getestet, bevor sie committed werden.
