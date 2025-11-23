#include <SPI.h>
#include <Ethernet.h>

// CONNECTION
// Arduino nano --> w5500
// 3V --> V
// GND --> GND
// D9 --> RST
// D10 --> CS
// D11 --> MO
// D12 --> MI
// D13 --> SCK

byte mac[6];

void generateUniqueMAC() {
  // Vendor prefix: 24:6F:28 (ESP32-like, ale legálny pre testovanie)
  mac[0] = 0x24;
  mac[1] = 0x6F;
  mac[2] = 0x28;

  // Posledné 3 bajty z millis() — unikátne po každom rešete
  uint32_t t = millis();
  mac[3] = (t >> 16) & 0xFF;
  mac[4] = (t >> 8)  & 0xFF;
  mac[5] = t & 0xFF;
}

void printMAC(const byte* m) {
  for (int i = 0; i < 6; i++) {
    if (i > 0) Serial.print(":");
    if (m[i] < 0x10) Serial.print("0");
    Serial.print(m[i], HEX);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("🔍 Arduino Nano + W5500 — Inteligentný test pripojenia");
  Serial.println("====================================================");

  // 1. Vygeneruj unikátnu MAC
  generateUniqueMAC();
  Serial.print("Vygenerovaná MAC: "); printMAC(mac); Serial.println();

  // 2. DHCP pokus
  Serial.print("🔄 DHCP pokus... ");
  bool dhcpSuccess = (Ethernet.begin(mac) != 0);
  if (dhcpSuccess) {
    Serial.println("✅ OK");
  } else {
    Serial.println("❌ ZLYHALO");
  }

  // 3. Základné údaje (aj keď DHCP zlyhá, môžeme mať link)
  IPAddress localIP = Ethernet.localIP();
  IPAddress gateway = Ethernet.gatewayIP();

  Serial.print("IP adresa:        ");
  if (localIP != IPAddress(0,0,0,0)) {
    Serial.println(localIP);
  } else {
    Serial.println("0.0.0.0 (žiadna IP)");
  }

  Serial.print("Brána:            ");
  if (gateway != IPAddress(0,0,0,0)) {
    Serial.println(gateway);
  } else {
    Serial.println("0.0.0.0 (žiadna brána)");
  }

  // 4. LAN link (kábel zapojený?)
  Serial.print("LAN kábel:        ");
  bool linkUp = (Ethernet.linkStatus() == LinkON);
  if (linkUp) {
    Serial.println("✅ ZAPOJENÝ");
  } else {
    Serial.println("❌ NEZAPOJENÝ / BEZ LINKU");
  }

  // 5. Test brány (len ak máme IP a link)
  Serial.print("Brána dostupná:   ");
  bool gatewayReachable = false;
  if (dhcpSuccess && linkUp && gateway != IPAddress(0,0,0,0)) {
    EthernetClient client;
    // Pokus o pripojenie na port 53 (DNS) alebo 80 — rýchly, bez odosielania dát
    if (client.connect(gateway, 53)) {
      client.stop();
      gatewayReachable = true;
      Serial.println("✅ ÁNO");
    } else if (client.connect(gateway, 80)) {
      client.stop();
      gatewayReachable = true;
      Serial.println("✅ ÁNO (port 80)");
    } else {
      Serial.println("❌ NIE");
    }
  } else {
    Serial.println("–");
  }

  // 6. Finálny záver
  Serial.println("\n────────────────────────────────────────────────────");
  Serial.print("VÝSLEDOK: ");
  if (dhcpSuccess && linkUp && gatewayReachable) {
    Serial.println("✅ Pripojené k sieti — všetko funguje!");
  } else if (linkUp && dhcpSuccess) {
    Serial.println("⚠️  Pripojené, ale brána neodpovedá (sieť bez internetu?)");
  } else if (linkUp) {
    Serial.println("⚠️  LAN kábel OK, ale DHCP zlyhal (skontroluj router)");
  } else {
    Serial.println("❌ Nepripojené — skontroluj kábel a napájanie W5500");
  }
  Serial.println("────────────────────────────────────────────────────");
}

void loop() {
  // Nič — test je jednorázový
}
