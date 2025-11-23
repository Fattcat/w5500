#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

// MAC adresa (ľubovoľná, ale jedinečná v sieti)
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// IP adresa (statická, alebo DHCP ak chceš — tu statická pre jednoduchosť)
IPAddress ip(192, 168, 1, 100);
EthernetServer server(80);

const int sd_cs = 4;  // SD karty CS pin

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); // pre Leonardo/Pro Micro, ale Nano to prežije

  // Inicializácia SD karty
  Serial.print("Intialized SD... ");
  if (!SD.begin(sd_cs)) {
    Serial.println("❌ FAILED");
    while (1) delay(1000);
  }
  Serial.println("✅ OK");

  // Skontroluj, či existuje index.html
  if (!SD.exists("/index.html")) {
    Serial.println("❗ Varovanie: /index.html neexistuje na SD!");
  }

  // Ethernet
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("🌐 Server beží na http://");
  Serial.println(Ethernet.localIP());
}

// MIME typy podľa prípony
String getContentType(const String& filename) {
  if (filename.endsWith(".htm")) return "text/html";
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".json")) return "application/json";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".jpg")) return "image/jpeg";
  if (filename.endsWith(".jpeg")) return "image/jpeg";
  if (filename.endsWith(".gif")) return "image/gif";
  if (filename.endsWith(".svg")) return "image/svg+xml";
  if (filename.endsWith(".ico")) return "image/x-icon";
  return "application/octet-stream";
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n') break; // koniec hlavičky
      }
    }

    // Extrahuj cestu z GET /cesta HTTP/1.1
    int startIndex = request.indexOf(' ') + 1;
    int endIndex = request.indexOf(' ', startIndex);
    String path = request.substring(startIndex, endIndex);
    if (path == "/") path = "/index.html";

    // Kontrola existencie
    if (!SD.exists(path)) {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("404: Súbor neexistuje");
      client.stop();
      return;
    }

    // Načítaj súbor a odoslaj
    File file = SD.open(path, FILE_READ);
    if (!file) {
      client.println("HTTP/1.1 500 Internal Error");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("500: Chyba pri čítaní súboru");
    } else {
      client.println("HTTP/1.1 200 OK");
      client.print("Content-Type: ");
      client.println(getContentType(path));
      client.println("Connection: close");
      client.println(); // prázdny riadok = koniec hlavičky

      // Streamuj obsah → šetrí RAM!
      byte buffer[64];
      int len;
      while ((len = file.read(buffer, sizeof(buffer))) > 0) {
        client.write(buffer, len);
      }
      file.close();
    }
    client.stop();
  }
}
