#include <Arduino.h>  // Core Arduino runtime (setup/loop, Serial utilities)
#include <WiFi.h>     // ESP32 WiFi client API

#define TINY_GSM_MODEM_SIM7000SSL
#define TINY_GSM_RX_BUFFER 1024
#include <TinyGsmClient.h>  // Cellular modem helper

namespace {
// Hard-coded WiFi credentials for the allotment network.
constexpr char kSsid[] = "Tapo-2";
constexpr char kPassword[] = "dul6aCTv&seu";
// How long we keep trying to join the network before giving up.
constexpr unsigned long kConnectTimeoutMs = 20000;  // 20 seconds

// Cellular settings.
constexpr char kApn[] = "infisim.iot";
constexpr char kSimPin[] = "";
constexpr unsigned long kNetworkAttachTimeoutMs = 120000;
constexpr unsigned long kPdpAttachTimeoutMs = 90000;
constexpr uint32_t kModemBaud = 115200;

// T-SIM7000G pin map.
constexpr int8_t kModemTxPin = 27;
constexpr int8_t kModemRxPin = 26;
constexpr int8_t kModemPwrKeyPin = 4;
constexpr int8_t kModemResetPin = 5;
constexpr int8_t kModemPowerEnPin = 12;  // Also lights the blue indicator LED.
constexpr int8_t kModemDtrPin = 25;
}  // namespace

enum class NetworkBackend { kNone, kWifi, kCellular };

#define SerialAT Serial1
TinyGsm modem(SerialAT);

NetworkBackend gActiveBackend = NetworkBackend::kNone;

const __FlashStringHelper *SimStatusToString(SimStatus status) {
  switch (status) {
    case SIM_READY:
      return F("ready");
    case SIM_LOCKED:
      return F("locked");
    case SIM_ANTITHEFT_LOCKED:
      return F("anti-theft locked");
    case SIM_ERROR:
    default:
      return F("error/missing");
  }
}

void waitForSerial() {
  Serial.begin(115200);  // Start serial port so the USB monitor works.

  const unsigned long start = millis();
  while (!Serial && millis() - start < 5000) {
    delay(10);
  }
}

bool networkVisible(const char *ssid) {
  Serial.println(F("Scanning for WiFi networks..."));  // F() keeps text in flash.

  // Disconnect first so we perform a full scan instead of reusing cached data.
  WiFi.disconnect(true);
  delay(100);

  // scanNetworks(false, true) = synchronous scan, include hidden SSIDs.
  const int found = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
  if (found < 0) {
    Serial.printf("Scan failed with error: %d\n", found);
    return false;
  }

  Serial.printf("Found %d network(s).\n", found);
  for (int i = 0; i < found; ++i) {
    if (WiFi.SSID(i) == ssid) {
      Serial.println(F("Target network detected."));
      return true;
    }
  }

  Serial.println(F("Target network not found."));
  return false;
}

bool connectToWifi(const char *ssid, const char *password) {
  Serial.printf("Connecting to %s", ssid);  // Print without newline.
  WiFi.begin(ssid, password);               // Kick off the connection attempt.

  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < kConnectTimeoutMs) {
    Serial.print('.');  // Simple progress feedback while waiting.
    delay(500);         // Poll twice per second.
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WiFi connected."));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    return true;
  }

  Serial.println(F("Failed to connect within timeout."));
  return false;
}

bool validPin(int8_t pin) {
  return pin >= 0;
}

void configureModemPins() {
  static bool initialized = false;

  if (!initialized) {
    if (validPin(kModemPowerEnPin)) {
      pinMode(kModemPowerEnPin, OUTPUT);
      digitalWrite(kModemPowerEnPin, HIGH);
    }
    if (validPin(kModemPwrKeyPin)) {
      pinMode(kModemPwrKeyPin, OUTPUT);
      digitalWrite(kModemPwrKeyPin, HIGH);
    }
    if (validPin(kModemResetPin)) {
      pinMode(kModemResetPin, OUTPUT);
      digitalWrite(kModemResetPin, HIGH);
    }
    if (validPin(kModemDtrPin)) {
      pinMode(kModemDtrPin, OUTPUT);
      digitalWrite(kModemDtrPin, LOW);  // LOW keeps modem awake
    }
    initialized = true;
    delay(50);
  } else if (validPin(kModemPowerEnPin)) {
    digitalWrite(kModemPowerEnPin, HIGH);
  }
}

void pulseModemPwrKey() {
  if (!validPin(kModemPwrKeyPin)) {
    delay(1100);
    return;
  }
  digitalWrite(kModemPwrKeyPin, LOW);
  delay(1100);
  digitalWrite(kModemPwrKeyPin, HIGH);
  delay(500);
}

bool ensureModemAwake() {
  configureModemPins();

  static bool serialStarted = false;
  if (!serialStarted) {
    SerialAT.begin(kModemBaud, SERIAL_8N1, kModemRxPin, kModemTxPin);
    serialStarted = true;
    delay(10);
  }

  if (modem.testAT()) {
    return true;
  }

  Serial.println(F("Modem not responding, toggling PWRKEY..."));
  pulseModemPwrKey();

  for (uint8_t attempt = 0; attempt < 10; ++attempt) {
    if (modem.testAT()) {
      return true;
    }
    delay(300);
  }
  return false;
}

bool bringUpPdpContext() {
  if (modem.isGprsConnected()) {
    modem.gprsDisconnect();
    delay(200);
  }

  Serial.print(F("Activating APN "));
  Serial.println(kApn);

  const unsigned long start = millis();
  do {
    if (modem.gprsConnect(kApn)) {
      return true;
    }
    Serial.println(F("APN attach failed, retrying..."));
    delay(2000);
  } while (millis() - start < kPdpAttachTimeoutMs);

  return false;
}

bool connectToCellular() {
  Serial.println(F("Attempting cellular connection via SIM7000..."));
  if (!ensureModemAwake()) {
    Serial.println(F("Modem failed to respond to wake-up sequence."));
    return false;
  }

  const char *simPin = (kSimPin[0] == '\0') ? nullptr : kSimPin;
  if (!modem.restart(simPin)) {
    Serial.println(F("Unable to initialize the modem."));
    return false;
  }

  const SimStatus simStatus = modem.getSimStatus();
  Serial.print(F("SIM status: "));
  Serial.println(SimStatusToString(simStatus));
  if (simStatus == SIM_LOCKED && simPin == nullptr) {
    Serial.println(F("SIM is locked and no PIN was provided."));
  } else if (simStatus == SIM_ERROR) {
    Serial.println(F("SIM missing or not detected."));
  }

  // Configure network mode for IoT connectivity
  // AUTO mode allows modem to select best available (2G/Cat-M/NB-IoT)
  Serial.println(F("Configuring network mode..."));
  if (!modem.setNetworkMode(MODEM_NETWORK_AUTO)) {
    Serial.println(F("Failed to set network mode"));
  }

  // Set preferred mode to Cat-M and NB-IoT for IoT SIM cards
  // This gives priority to LTE-M (Cat-M1) and NB-IoT over 2G
  if (!modem.setPreferredMode(MODEM_PREFERRED_CATM_NBIOT)) {
    Serial.println(F("Failed to set preferred mode"));
  }

  // Configure APN BEFORE network registration (required for some IoT carriers)
  Serial.print(F("Setting APN to: "));
  Serial.println(kApn);
  modem.sendAT(GF("+CGDCONT=1,\"IP\",\""), kApn, "\"");
  if (modem.waitResponse() != 1) {
    Serial.println(F("Failed to set APN - registration may fail"));
  }

  // Wait for network registration with detailed status reporting
  Serial.println(F("Waiting for network registration..."));
  const unsigned long regStart = millis();
  RegStatus status = REG_NO_RESULT;

  while ((status == REG_NO_RESULT || status == REG_SEARCHING || status == REG_UNREGISTERED) &&
         (millis() - regStart < kNetworkAttachTimeoutMs)) {
    status = modem.getRegistrationStatus();
    int16_t csq = modem.getSignalQuality();

    Serial.print(F("["));
    Serial.print((millis() - regStart) / 1000);
    Serial.print(F("s] Status: "));

    switch (status) {
      case REG_UNREGISTERED:
        Serial.print(F("not registered"));
        break;
      case REG_SEARCHING:
        Serial.print(F("searching"));
        break;
      case REG_DENIED:
        Serial.println(F("DENIED - Check APN and carrier compatibility!"));
        return false;
      case REG_OK_HOME:
        Serial.println(F("registered (home network)"));
        break;
      case REG_OK_ROAMING:
        Serial.println(F("registered (roaming)"));
        break;
      default:
        Serial.print(F("unknown ("));
        Serial.print(status);
        Serial.print(F(")"));
        break;
    }

    if (status != REG_OK_HOME && status != REG_OK_ROAMING) {
      Serial.print(F(", Signal: "));
      if (csq == 99 || csq < 0) {
        Serial.println(F("none/unknown"));
      } else {
        Serial.print(csq);
        Serial.print(F(" ("));
        Serial.print(-113 + (2 * csq));
        Serial.println(F(" dBm)"));
      }
      delay(2000);
    }
  }

  if (status != REG_OK_HOME && status != REG_OK_ROAMING) {
    Serial.println(F("Network registration failed!"));
    return false;
  }

  Serial.println(F("Network registered! Bringing up data session..."));
  if (!bringUpPdpContext()) {
    Serial.println(F("Failed to enable data on the configured APN."));
    return false;
  }

  Serial.print(F("Cellular connected. IP address: "));
  Serial.println(modem.localIP());
  return true;
}

void reportWifiStatus() {
  const wl_status_t status = WiFi.status();

  Serial.print(F("WiFi status: "));
  // Map the numeric status code into readable text.
  switch (status) {
    case WL_CONNECTED:
      Serial.print(F("connected, RSSI="));  // RSSI is the signal strength in dBm.
      Serial.print(WiFi.RSSI());
      Serial.print(F(" dBm, IP="));
      Serial.println(WiFi.localIP());
      break;
    case WL_CONNECT_FAILED:
      Serial.println(F("connect failed"));
      break;
    case WL_DISCONNECTED:
      Serial.println(F("disconnected"));
      break;
    case WL_CONNECTION_LOST:
      Serial.println(F("connection lost"));
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println(F("SSID unavailable"));
      break;
    default:
      Serial.println(static_cast<int>(status));
      break;
  }
}

void reportCellularStatus() {
  const bool registered = modem.isNetworkConnected();
  const bool pdpActive = modem.isGprsConnected();
  const int16_t csq = modem.getSignalQuality();

  Serial.print(F("Cellular status: "));
  Serial.print(registered ? F("registered") : F("searching"));
  Serial.print(F(", PDP="));
  Serial.print(pdpActive ? F("active") : F("inactive"));
  Serial.print(F(", RSSI="));
  if (csq == 99 || csq < 0) {
    Serial.print(F("unknown"));
  } else {
    const int16_t dbm = -113 + (2 * csq);
    Serial.print(dbm);
    Serial.print(F(" dBm (CSQ="));
    Serial.print(csq);
    Serial.print(')');
  }

  if (pdpActive) {
    Serial.print(F(", IP="));
    Serial.println(modem.localIP());
  } else {
    Serial.println();
  }
}

void setup() {
  // setup() runs once after the ESP32 boots.
  waitForSerial();
  Serial.println();
  Serial.println(F("Allotment project booting..."));

  WiFi.mode(WIFI_STA);  // Put WiFi into station (client) mode.

  bool wifiConnected = false;
  if (networkVisible(kSsid)) {
    wifiConnected = connectToWifi(kSsid, kPassword);
  } else {
    Serial.println(F("WiFi network unavailable. Falling back to cellular."));
  }

  if (wifiConnected) {
    gActiveBackend = NetworkBackend::kWifi;
  } else if (connectToCellular()) {
    gActiveBackend = NetworkBackend::kCellular;
  } else {
    gActiveBackend = NetworkBackend::kNone;
    Serial.println(F("Unable to establish WiFi or cellular connectivity."));
  }
}

void loop() {
  // loop() runs forever; we use it to report health every 10 seconds.
  static unsigned long lastReport = 0;
  const unsigned long now = millis();

  if (now - lastReport >= 10000) {
    lastReport = now;

    switch (gActiveBackend) {
      case NetworkBackend::kWifi:
        reportWifiStatus();
        break;
      case NetworkBackend::kCellular:
        reportCellularStatus();
        break;
      case NetworkBackend::kNone:
      default:
        Serial.println(F("No network connection active."));
        break;
    }
  }
}
