#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SS_PIN  5    // Pino SDA/SS do RC522
#define RST_PIN 27   // Pino RST do RC522

MFRC522 rfid(SS_PIN, RST_PIN);

// wifi
const char* ssid = "";
const char* password = "";

// url
// worker-teste-kv.vitorserra
const char* serverUrl = "https://xxx.workers.dev";
const char* espId = "2";

// Função que será chamada quando detectar um cartão
void minhaFuncao(String uid) {
  Serial.print("UID: ");
  Serial.println(uid);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String fullUrl = String(serverUrl) + "/check?mac=" + uid + "&esp_id=" + espId;
    http.begin(fullUrl);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("Status: ");
      Serial.println(httpResponseCode);
      String resposta = http.getString();
      Serial.println("Resposta:");
      Serial.println(resposta);
    } else {
      Serial.print("Erro: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi desconectado!");
  }

  Serial.println("");
}

void setup() {
  Serial.begin(115200);
  SPI.begin(); 
  rfid.PCD_Init(); // Inicializa o RC522
  Serial.println("Aproxime o cartão RFID/NFC...");
  // wifi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

}

void loop() {
  // Verifica se há um novo cartão
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Verifica se consegue ler o cartão
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Monta o UID como string (formato semelhante a MAC: XX:XX:XX:XX:XX)
  String uidStr = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (i != 0) uidStr += ":";
    if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(rfid.uid.uidByte[i], HEX);
  }
  uidStr.toUpperCase();

  // Chama sua função, passando o "MAC"
  Serial.println("");
  minhaFuncao(uidStr);

  // Para evitar múltiplas leituras rápidas
  delay(1000);

  // Finaliza a leitura do cartão
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
