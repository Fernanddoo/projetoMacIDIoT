#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>

// --- CONFIGURAÇÕES - ALTERE ESTES VALORES ---
const char* ssid = ""; // --- Nome da Rede
const char* password = ""; // --- Senha do WiFi
const char* serverUrl = ""; // <-- SUA URL AQUI do cloudflare

// ID fixo para este ESP32, para registrar outro ESP32, altere o valor
const char* ESP_ID = "1";

// Pinos para o leitor MFRC522
#define SS_PIN    5  // SDA / SS (Slave Select)
#define RST_PIN   27 // RST (Reset)
// --- FIM DAS CONFIGURAÇÕES ---

// Cria a instância do leitor RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);
// Cria a instância do cliente HTTP
HTTPClient http;

// Função para conectar ao Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Função para formatar o UID do cartão em uma String estilo MAC
String getMacFromUID() {
  String mac = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    // Adiciona um zero à esquerda se o valor for menor que 0x10 (16)
    if (mfrc522.uid.uidByte[i] < 0x10) {
      mac += "0";
    }
    mac += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) {
      mac += ":";
    }
  }
  mac.toUpperCase();
  return mac;
}

// Função para registrar a tag no servidor
void registerTag(String macAddress) {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nEnviando requisição de registro...");

    // Cria o documento JSON
    JsonDocument doc;
    doc["esp_id"] = ESP_ID;
    doc["mac"] = macAddress;

    String jsonPayload;
    serializeJson(doc, jsonPayload);
    Serial.print("Payload: ");
    Serial.println(jsonPayload);

    // Inicia a requisição HTTP
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Envia a requisição POST
    int httpCode = http.POST(jsonPayload);

    // Trata a resposta
    if (httpCode > 0) {
      String response = http.getString();
      Serial.print("Código de Status HTTP: ");
      Serial.println(httpCode);
      Serial.print("Resposta do servidor: ");
      Serial.println(response);

      if (httpCode == 201) {
        Serial.println("==> SUCESSO: Tag registrada com sucesso! <==");
      } else if (httpCode == 409) {
        Serial.println("==> AVISO: Esta combinação de Tag e ESP já foi registrada. <==");
      } else {
        Serial.println("==> ERRO: O servidor retornou um erro inesperado. <==");
      }
      
    } else {
      Serial.printf("[HTTP] Falha no POST, erro: %s\n", http.errorToString(httpCode).c_str());
    }

    // Libera os recursos
    http.end();
  } else {
    Serial.println("Erro: Sem conexão com o WiFi.");
  }
}

void setup() {
  Serial.begin(115200);
  
  setup_wifi(); // Conecta ao Wi-Fi

  SPI.begin();      // Inicia a comunicação SPI
  mfrc522.PCD_Init(); // Inicia o leitor MFRC522
  
  delay(4);
  mfrc522.PCD_DumpVersionToSerial(); // Mostra detalhes do firmware do leitor
  Serial.println("\nSistema pronto. Aproxime uma tag RFID para registrá-la...");
}

void loop() {
  // Procura por novos cartões
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Seleciona um dos cartões
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Se chegou até aqui, um cartão foi lido com sucesso
  Serial.println("---------------------------------");
  Serial.println("Nova Tag Detectada!");

  // Obtém o MAC (UID) formatado
  String macAddress = getMacFromUID();
  Serial.print("UID da Tag: ");
  Serial.println(macAddress);

  // Envia os dados para o servidor
  registerTag(macAddress);

  // Instrui o cartão a parar a comunicação para não ser lido novamente no mesmo instante
  mfrc522.PICC_HaltA();
  
  Serial.println("---------------------------------");
  Serial.println("\nAproxime a próxima tag...");

  // Aguarda um pouco antes da próxima leitura
  delay(500); 
}
