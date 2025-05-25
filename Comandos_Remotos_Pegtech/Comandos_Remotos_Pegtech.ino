// ------------------- Definição de Pinos -------------------
#define RXD2 32
#define TXD2 33
#define rele 26
#define botao 25

// ------------------- Inclusão de Bibliotecas -------------------
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ------------------- Configuração de Variáveis -------------------
String inputString = "";
String numString = "";

const int thisLockerId = 2;

String savedCodeBar = "";
String savedKey = "";

int trigger_timer = 2000;
bool respondedToAvailableRequest = false;

// ------------------- Configurações Wi-Fi e MQTT -------------------
const char* ssid = "<Nome da Rede>";
const char* password = "<Senha da Rede>";

const char* mqtt_server = "<Host do Broker>";
const int mqtt_port = 8883;
const char* mqtt_user = "<credencial do Broker";
const char* mqtt_password = "<Senha do Brocker>";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ------------------- Certificado CA -------------------
const char* ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
"-----END CERTIFICATE-----\n";

// ------------------- Setup -------------------
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(rele, OUTPUT);
  pinMode(botao, INPUT);

  setup_wifi();
  espClient.setCACert(ca_cert);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();

  Serial.println("Sistema unificado iniciado.");
}

// ------------------- Wi-Fi e MQTT -------------------
void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000); Serial.print(".");
  }
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      client.subscribe("locker/available");
      client.subscribe("locker/package/register");
      Serial.println("MQTT conectado.");
    } else {
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  Serial.printf("📨 Mensagem [%s]: %s\n", topic, message.c_str());

  if (strcmp(topic, "locker/available") == 0 && !respondedToAvailableRequest) {
    StaticJsonDocument<128> doc;
    if (!deserializeJson(doc, message) && doc["locker_id"] == thisLockerId) {
      publishAvailableResponse();
      respondedToAvailableRequest = true;
    }
  }

  if (strcmp(topic, "locker/package/register") == 0) {
    StaticJsonDocument<256> doc;
    if (!deserializeJson(doc, message)) {
      JsonObject pkg = doc["package"];
      if (pkg["locker_id"] == thisLockerId) {
        savedCodeBar = String((const char*)pkg["package_code"]);
        savedKey = String((const char*)pkg["package_pickup_password"]);
        Serial.printf("📦 Código: %s | Senha: %s\n", savedCodeBar.c_str(), savedKey.c_str());
        respondedToAvailableRequest = false;
      }
    }
  }
}

void publishAvailableResponse() {
  StaticJsonDocument<128> doc;
  doc["locker_id"] = thisLockerId;
  JsonArray ports = doc.createNestedArray("ports");
  ports.add(1); ports.add(3); ports.add(5);
  char buffer[128];
  serializeJson(doc, buffer);
  client.publish("locker/available", buffer);
  Serial.println("📤 Locker disponível publicado.");
}

// ------------------- Processamento Serial2 (tela touch) -------------------
void processSerial2Data() {
  while (Serial2.available()) {
    char receivedChar = (char)Serial2.read();
    inputString += receivedChar;

    if (inputString.endsWith("out") || inputString.endsWith("rec")) {
      bool isOut = inputString.endsWith("out");
      numString = inputString.substring(0, inputString.length() - 3);
      inputString = "";

      uint32_t value = numString.toInt();
      String strValue = String(value);

      if (isOut && validarSenhaKey(strValue)) {
        acionarRele();
        Serial.println("🔓 Senha válida - Porta aberta.");
        publishPickupConfirm();  // ← Publica confirmação de retirada
      } else if (!isOut && validarSenhaCodeBar(strValue)) {
        acionarRele();
        Serial.println("🔓 Código válido - Porta aberta.");
        publishRegisterConfirm();  // ← Publica confirmação de depósito
      } else {
        Serial.println("❌ Código/Senha incorreto.");
      }
    }
  }
}

// ------------------- Validação e Abertura da Porta -------------------
bool validarSenhaCodeBar(String codeBar) {
  return codeBar == savedCodeBar;
}

bool validarSenhaKey(String key) {
  return key == savedKey;
}

void acionarRele() {
  digitalWrite(rele, HIGH);
  delay(trigger_timer);
  digitalWrite(rele, LOW);
  Serial.println("🔒 Porta fechada.");
}

// ------------------- Publicações MQTT adicionais -------------------
void publishRegisterConfirm() {
  StaticJsonDocument<128> doc;
  doc["locker_id"] = thisLockerId;
  doc["package_code"] = savedCodeBar;
  doc["msg"] = "deposito concluido";

  char buffer[128];
  serializeJson(doc, buffer);
  client.publish("locker/package/register", buffer);
  Serial.println("📤 Confirmação de depósito publicada.");
}

void publishPickupConfirm() {
  StaticJsonDocument<128> doc;
  doc["locker_id"] = thisLockerId;
  doc["package_code"] = savedCodeBar;
  doc["msg"] = "retirada realizada";

  char buffer[128];
  serializeJson(doc, buffer);
  client.publish("locker/package/pickup", buffer);
  Serial.println("📤 Confirmação de retirada publicada.");
}

// ------------------- Loop Principal -------------------
void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  processSerial2Data();

  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'q') {
      acionarRele();
      Serial.println("🧪 Teste manual executado.");
    }
  }

  delay(100);
}
