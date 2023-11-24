#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// Configurações de WiFi
const char *SSID = "Wokwi-GUEST";
const char *PASSWORD = "";  // Substitua pelo sua senha

// Configurações de MQTT
const char *BROKER_MQTT = "broker.hivemq.com";
const int BROKER_PORT = 1883;
const char *ID_MQTT = "enzo";
const char *USERNAME = "";  // Deixe em branco se não for usar um nome de usuário MQTT
const char *TOPIC_SEND_VARIABLE = "estoque/distancia/iot";  // Tópico para enviar as variáveis


const int trigPin = 13;  // Pino de controle do transmissor ultrassônico
const int echoPin = 12; // Pino de leitura do receptor ultrassônico

int distanciaAnterior = 0;  // Variável para armazenar a última distância medida
int caixasPor10cm = 1; // Quantidade de caixas por variação de 10cm
int estoqueMaximo = 40;  // Número máximo de caixas
int estoqueAtual = estoqueMaximo;  // Inicializa o estoque atual com o máximo

// Variáveis globais
WiFiClient espClient;
PubSubClient MQTT(espClient);
unsigned long publishUpdate = 0;
const int TAMANHO = 200;
#define PUBLISH_DELAY 2000

void initWiFi() {
  Serial.print("Conectando com a rede: ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso: ");
  Serial.println(SSID);
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
}

void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("Tentando conectar com o Broker MQTT: ");
    Serial.println(BROKER_MQTT);

    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado ao broker MQTT!");
    } else {
      Serial.println("Falha na conexão com MQTT. Tentando novamente em 2 segundos.");
      delay(2000);
    }
  }
}

void checkWiFIAndMQTT() {
  if (WiFi.status() != WL_CONNECTED) reconnectWiFi();
  if (!MQTT.connected()) reconnectMQTT();
}

void reconnectWiFi(void)
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  // Conecta na rede WI-FI
  WiFi.begin(SSID, PASSWORD); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Wifi conectado com sucesso");
  Serial.print(SSID);
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(9600);  // Inicia a comunicação serial para monitoramento no Serial Monitor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  initWiFi();
  initMQTT();
}

void loop() {
  checkWiFIAndMQTT();
  MQTT.loop();

  // Gera um pulso no pino de controle do transmissor ultrassônico
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Lê a duração do pulso no pino de leitura do receptor ultrassônico
  int distanciaAtual = pulseIn(echoPin, HIGH) * 0.034 / 2;

  // Exibe a distância medida no monitor serial
  Serial.print("Distancia: ");
  Serial.print(distanciaAtual);
  Serial.print(" cm");

  // Calcula a variação na quantidade de caixas com base na mudança de 10cm
  int variacaoEstoque = (distanciaAnterior - distanciaAtual) / 10 * caixasPor10cm;

  // Atualiza o estoque atual de acordo com a variação calculada
  estoqueAtual += variacaoEstoque;

  // Garante que o estoque não ultrapasse o máximo nem seja menor que zero
  if (estoqueAtual > estoqueMaximo) {
    estoqueAtual = estoqueMaximo;
  } else if (estoqueAtual < 0) {
    estoqueAtual = 0;
  }

  // Exibe a variação no estoque no monitor serial
  if (variacaoEstoque != 0) {
    Serial.print(" - Variação no estoque: ");
    Serial.print(variacaoEstoque);
  }
  
  // Exibe o estoque atual no monitor serial
  Serial.print(" - Estoque Atual: ");
  Serial.println(estoqueAtual);

  // Atualiza a distância anterior para o próximo ciclo
  distanciaAnterior = distanciaAtual;

  // Aguarda um curto período antes da próxima leitura
  delay(500);

  

  if ((millis() - publishUpdate) >= PUBLISH_DELAY) {
    publishUpdate = millis();
    if (!isnan(estoqueAtual)) {
      // Enviar distancia
      StaticJsonDocument<TAMANHO> doc_distancia;
      doc_distancia["variable"] = "estoque";
      doc_distancia["unit"] = "peças";
      doc_distancia["value"] = estoqueAtual;

      char buffer_distancia[TAMANHO];
      serializeJson(doc_distancia, buffer_distancia);
      MQTT.publish(TOPIC_SEND_VARIABLE, buffer_distancia);
      Serial.println(buffer_distancia);
    }
  }
}