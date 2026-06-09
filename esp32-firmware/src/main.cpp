#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <TinyGPS++.h>
#include <time.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "secrets.h"

// --- DADOS DA REDE DA APRESENTAÇÃO ---
const char *ssid = "Redmi Note 13";
const char *password = "12345678";

const char *mqtt_server = "10.189.28.150";
const int mqtt_port = 8883; // Porta Segura mTLS

// ===== OBJETOS DE REDE E SENSORES =====
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Pinos do Semáforo
#define LED_VERMELHO 25
#define LED_AMARELO 26
#define LED_VERDE 27

// Sensor de Temperatura e Umidade
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// GPS
TinyGPSPlus gps;
HardwareSerial SerialGPS(2);

unsigned long tempoUltimaLeitura = 0;
const long intervaloLeitura = 10000; // Envia a cada 10 segundos

// --- FUNÇÕES AUXILIARES ---

void apagarLeds()
{
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERDE, LOW);
}

void conectarWiFi()
{
  Serial.print("Conectando WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP do ESP32 na rede: ");
  Serial.println(WiFi.localIP());
}

void sincronizarRelogio()
{
  Serial.print("Sincronizando relógio via internet (NTP)...");
  // Fuso horário: -10800 segundos = GMT-3 (Brasília)
  configTime(-10800, 0, "pool.ntp.org", "time.windows.com");

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo))
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nRelógio atualizado com sucesso!");
  Serial.println(&timeinfo, "Data e Hora do ESP32: %d/%m/%Y %H:%M:%S");
}

void conectarMQTT()
{
  Serial.println("Conectando MQTT TLS (Porta 8883)...");

  while (!client.connected())
  {
    apagarLeds();
    digitalWrite(LED_AMARELO, HIGH);

    // Agora enviamos o DEVICE_ID também no terceiro parâmetro (que é a senha)
    if (client.connect(DEVICE_ID, DEVICE_ID, DEVICE_ID))
    {
      Serial.print("MQTT Seguro Conectado! JITP Aprovado para: ");
      Serial.println(DEVICE_ID);
      apagarLeds();
    }
    else
    {
      Serial.print("Falha MQTT, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5s...");
      apagarLeds();
      digitalWrite(LED_VERMELHO, HIGH);
      delay(5000);
    }
  }
}

void configurarOTA()
{
  // Define o nome que vai aparecer na rede (pode ser o DEVICE_ID)
  ArduinoOTA.setHostname(DEVICE_ID);

  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("Iniciando atualização OTA: " + type);
    // Apaga os LEDs pra indicar que tá gravando
    apagarLeds(); });

  ArduinoOTA.onEnd([]()
                   { Serial.println("\nAtualização Concluída!"); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progresso: %u%%\r", (progress / (total / 100))); });

  ArduinoOTA.onError([](ota_error_t error)
                     {
    Serial.printf("Erro OTA [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Falha de Autenticação");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha no Início");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha de Conexão");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha de Recebimento");
    else if (error == OTA_END_ERROR) Serial.println("Falha no Fim"); });

  ArduinoOTA.begin();
  Serial.println("OTA Configurado e aguardando...");
}

void setup()
{
  Serial.begin(115200);
  SerialGPS.begin(9600, SERIAL_8N1, 16, 17);
  dht.begin();

  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  apagarLeds();

  digitalWrite(LED_AMARELO, HIGH); // Indica que está iniciando

  conectarWiFi();
  sincronizarRelogio();

  // Inicia o OTA logo depois de conectar na rede!
  configurarOTA();
  espClient.setCACert(root_ca);          // Valida a identidade do EMQX
  espClient.setCertificate(client_cert); // Entrega o crachá VIP do caminhão
  espClient.setPrivateKey(client_key);   // Senha (chave privada) do caminhão

  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  conectarMQTT();
}

void loop()
{
  ArduinoOTA.handle(); // Mantém o OTA escutando

  if (!client.connected())
  {
    conectarMQTT();
  }
  client.loop();

  // Processa dados da antena do GPS
  while (SerialGPS.available() > 0)
  {
    gps.encode(SerialGPS.read());
  }

  // Timer de envio sem usar delay()
  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoUltimaLeitura >= intervaloLeitura)
  {
    tempoUltimaLeitura = tempoAtual;

    float humidade = dht.readHumidity();
    float temperatura = dht.readTemperature();

    // Trava de segurança do DHT
    if (isnan(humidade) || isnan(temperatura))
    {
      Serial.println("Erro de leitura no DHT22! Verifique os cabos.");
      apagarLeds();
      digitalWrite(LED_VERMELHO, HIGH);
      return;
    }

    // Fallback de coordenadas caso esteja sem céu visível (dentro de sala)
    float lat_envio = -23.550520;
    float lng_envio = -46.633308;
    bool gps_sem_sinal = true;

    if (gps.location.isValid())
    {
      lat_envio = gps.location.lat();
      lng_envio = gps.location.lng();
      gps_sem_sinal = false;
    }
    else
    {
      Serial.println("GPS sem sinal. Fallback para coordenadas de teste.");
    }

    // Montagem do JSON dinâmico
    String jsonPayload = "{\"identificador\":\"";
    jsonPayload += DEVICE_ID;
    jsonPayload += "\", \"temp\": " + String(temperatura) + ", ";
    jsonPayload += "\"umid\": " + String(humidade) + ", ";
    jsonPayload += "\"lat\": " + String(lat_envio, 6) + ", ";
    jsonPayload += "\"lng\": " + String(lng_envio, 6) + "}";

    // Publicação
    String topico = String("telemetria/") + DEVICE_ID;
    Serial.print("Publicando no tópico [");
    Serial.print(topico);
    Serial.print("]: ");
    Serial.println(jsonPayload);

    client.publish(topico.c_str(), jsonPayload.c_str());

    // Feedback visual do status de envio e GPS
    apagarLeds();
    if (gps_sem_sinal)
    {
      digitalWrite(LED_AMARELO, HIGH); // Sucesso na rede, mas sem precisão espacial
    }
    else
    {
      digitalWrite(LED_VERDE, HIGH); // Sucesso total (mTLS + GPS OK)
    }

    Serial.println("-----------------------------------");
  }
}