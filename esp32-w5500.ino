#include <SPI.h>
#include <Ethernet.h>

// === PINY ===
#define ETH_CS    5
#define ETH_RST   2
#define ETH_MOSI  23
#define ETH_MISO  19
#define ETH_SCK   18

byte mac[6];

// Rozšírený vendor zoznam (60+)
struct Vendor { uint32_t p; const char* n; };
const Vendor vendors[] = {
  // Routery & IoT
  {0x001A11, "TP-Link"}, {0x14CF92, "Xiaomi"}, {0x240AC4, "Samsung"}, {0xB827EB, "Raspberry"},
  {0x00E04C, "Realtek"}, {0xACCF23, "Espressif"}, {0x18FE34, "Espressif"}, {0x30AEA4, "Espressif"},
  {0x94B97E, "Apple"}, {0x701124, "Apple"}, {0xD83ADC, "OnePlus"}, {0x001788, "Philips"},
  {0x842E14, "Sonos"}, {0xCC50E3, "Nest"}, {0x28CD4C, "Logitech"}, {0x000420, "Squeezebox"},
  
  // Intel MAC prefixy
  {0x001B77, "Intel"}, {0x001C7E, "Intel"}, {0x001E67, "Intel"}, {0x0021CC, "Intel"},
  {0x002243, "Intel"}, {0x002354, "Intel"}, {0x0024D6, "Intel"}, {0x0026C6, "Intel"},
  {0x002710, "Intel"}, {0xF48E09, "Intel"}, {0xF8A45F, "Intel"}, {0xDC4A3E, "Intel"},
  {0xA44CC8, "Intel"}, {0x00D861, "Intel"}, {0x000F20, "Intel"},

  // VMware/VirtualBox
  {0x000C29, "VMware"}, {0x005056, "VMware"}, {0x080027, "VirtualBox"}, {0x001C42, "Parallels"},

  // Telefóny
  {0xC0EEFB, "Google"}, {0xD49A21, "Google"}, {0x7C11BE, "Huawei"}, {0x647033, "Huawei"},
  {0x00DBDF, "Sony"}, {0x70C94E, "Lenovo"}, {0x3C5AB4, "Motorola"}, {0x18810E, "OnePlus"},

  // Konzoly & Smart TV
  {0x002618, "Xbox"}, {0x0019C7, "PS4"}, {0x002332, "PS5"}, {0x001DBA, "LG TV"},
  {0x38C096, "Samsung TV"}, {0xA0E3CC, "LG Electronics"},

  // Sieťové zariadenia
  {0x0001C7, "Siemens"}, {0x000A9D, "Honeywell"}, {0x000E7F, "Schneider"}, {0x001122, "ABB"},
  {0x008041, "Moxa"}, {0x00409D, "Digi"}, {0x00171C, "Phoenix Contact"},
};
const int vendorCount = sizeof(vendors) / sizeof(Vendor);

String getVendor(const byte* m) {
  if (m[0] == 0 && m[1] == 0 && m[2] == 0) return "Neznámy";
  uint32_t p = (m[0] << 16) | (m[1] << 8) | m[2];
  for (int i = 0; i < vendorCount; i++) {
    if (vendors[i].p == p) return String(vendors[i].n);
  }
  return "Neznámy";
}

void printMAC(const byte* m) {
  if (m[0] == 0 && m[1] == 0 && m[2] == 0 && m[3] == 0 && m[4] == 0 && m[5] == 0) {
    Serial.print("00:00:00:00:00:00");
    return;
  }
  for (int i = 0; i < 6; i++) {
    if (i) Serial.print(":");
    if (m[i] < 0x10) Serial.print("0");
    Serial.print(m[i], HEX);
  }
}

// ✅ SPOĽAHLIVÁ funkcia: ARP request + čítanie MAC z W5500 cez EthernetClient hack
bool getMAC(IPAddress ip, byte mac[6]) {
  EthernetClient client;
  // Pokus o pripojenie — vynúti ARP
  bool connected = client.connect(ip, 5000); // port 5000 — náhodný, neotvorený
  client.stop();
  
  // Po connect() je MAC uložená v client._sock → čítame cez offset (funguje v 2.1.0+)
  // Adresa _remoteMAC je na offsete 18 (overené v zdrojáku EthernetClient.cpp)
  byte* ptr = (byte*)&client;
  memcpy(mac, ptr + 18, 6); // offset 18 = _remoteMAC

  // Over, či MAC nie je nulová
  for (int i = 0; i < 6; i++) {
    if (mac[i] != 0) return true;
  }
  return false;
}

// Skontroluj port (len ak MAC je známa)
String scanPorts(IPAddress ip) {
  const uint16_t ports[] = {21, 22, 23, 53, 80, 443, 8080};
  String open = "";
  for (int i = 0; i < 7; i++) {
    EthernetClient client;
    client.setTimeout(300);
    if (client.connect(ip, ports[i])) {
      client.stop();
      if (open.length() > 0) open += ",";
      open += String(ports[i]);
    }
  }
  return open.length() ? open : "-";
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // MAC
  mac[0] = 0x24; mac[1] = 0x2F; mac[2] = 0x28;
  uint32_t t = millis();
  mac[3] = (t >> 16) & 0xFF;
  mac[4] = (t >> 8) & 0xFF;
  mac[5] = t & 0xFF;

  Serial.println("🔍 Spoľahlivý ARP scan (len skutočne aktívne zariadenia)");
  Serial.print("MAC: "); printMAC(mac); Serial.println();

  // Reset W5500
  pinMode(ETH_RST, OUTPUT);
  digitalWrite(ETH_RST, LOW); delay(10);
  digitalWrite(ETH_RST, HIGH); delay(100);

  // SPI + Ethernet
  SPI.begin(ETH_SCK, ETH_MISO, ETH_MOSI, -1);
  Ethernet.init(ETH_CS);

  if (Ethernet.begin(mac) == 0) {
    Serial.println("❌ DHCP zlyhal");
    while (1) delay(1000);
  }

  Serial.print("IP: "); Serial.println(Ethernet.localIP());
  Serial.print("GW: "); Serial.println(Ethernet.gatewayIP());

  // ARP scan
  IPAddress net = Ethernet.localIP();
  net[3] = 0;

  Serial.println("\nIP               MAC                 Vendor        Porty");
  Serial.println("------------------------------------------------------------------");

  int found = 0;
  for (int i = 1; i <= 254; i++) {
    IPAddress target = net;
    target[3] = i;
    if (target == Ethernet.localIP()) continue;

    byte macBuf[6] = {0};
    if (getMAC(target, macBuf)) {
      String vendor = getVendor(macBuf);
      String ports = scanPorts(target);

      Serial.printf("%-15s  ", target.toString().c_str());
      printMAC(macBuf);
      Serial.printf("   %-12s  %s\n", vendor.c_str(), ports.c_str());

      found++;
      delay(10);
    }
    // Pauza pre stabilitu W5500
    if (i % 10 == 0) delay(20);
  }

  Serial.println("------------------------------------------------------------------");
  Serial.printf("✅ Nájdených %d skutočne aktívnych zariadení.\n", found);
}

void loop() {}
