#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Configuración WiFi
const char* ssid = "";
const char* password = "";
const char* server = "http://api.thingspeak.com/update";
const char* apiKey = "GKTA8ID38R9DVV8O";

// Pines del multiplexor
#define S0 16  // GPIO16 (D0)
#define S1 5   // GPIO5 (D1)
#define S2 4   // GPIO4 (D2)
#define S3 0   // GPIO0 (D3)
#define SIG A0 // Entrada del multiplexor

// Servomotores
Servo horizontal;
Servo vertical;

// Posiciones iniciales en 0°
uint8_t servoh = 0;  // Posición inicial de rotación
uint8_t servov = 0;  // Posición inicial de inclinación

void setup() {
  Serial.begin(115200);
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");

  // Configurar pines del multiplexor
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SIG, INPUT);
  
  // Inicializar servos
  horizontal.attach(14); // GPIO14 (D5)
  vertical.attach(13);   // GPIO12 (D7)
  
  // Posicionar servos en la posición inicial
  horizontal.write(servoh);
  vertical.write(servov);
  delay(500);
}

// Función para leer un canal del multiplexor
uint16_t readMux(uint8_t channel) {
  digitalWrite(S0, bitRead(channel, 0));
  digitalWrite(S1, bitRead(channel, 1));
  digitalWrite(S2, bitRead(channel, 2));
  digitalWrite(S3, bitRead(channel, 3));
  delayMicroseconds(100); // Pequeño delay para estabilidad
  return analogRead(SIG);
}

void loop() {
  static unsigned long lastPrintTime = 0;
  unsigned long currentMillis = millis();
  
  // Leer valores de las fotorresistencias
  uint16_t lt = readMux(0); // Arriba izquierda
  uint16_t rt = readMux(1); // Arriba derecha
  uint16_t ld = readMux(2); // Abajo izquierda
  uint16_t rd = readMux(3); // Abajo derecha
  
  // Leer voltaje del panel solar desde el canal 4 del multiplexor
  uint16_t rawVoltage = readMux(4);
  float voltage = rawVoltage * (3.3 / 1023.0) * ((100.0 + 10.0) / 10.0); // Ajuste del divisor de voltaje para 24V
  
  // Evitar división por cero
  float current = (voltage > 0) ? (550.0 / 40.98) * (voltage / 40.98) : 0; // Corriente basada en VMP
  float power = voltage * current; // P = V * I

  constexpr uint8_t tol = 30; // Ajuste de tolerancia para evitar oscilaciones
  
  // Promedios de lectura
  uint16_t avt = (lt + rt) >> 1; // Luz arriba
  uint16_t avd = (ld + rd) >> 1; // Luz abajo
  uint16_t avl = (lt + ld) >> 1; // Luz izquierda
  uint16_t avr = (rt + rd) >> 1; // Luz derecha

  // Diferencias de luz
  int16_t dvert = avt - avd;
  int16_t dhoriz = avl - avr;

  // Imprimir valores cada 15 minutos en filas separadas
  if (currentMillis - lastPrintTime >= 900000) {
    Serial.println("Valores del sistema:");
    Serial.print("Inclinación del panel: "); Serial.print(servov); Serial.println("°");
    Serial.print("Rotación de la base: "); Serial.print(servoh); Serial.println("°");
    Serial.print("Voltaje del Panel: "); Serial.print(voltage, 2); Serial.println(" V");
    Serial.print("Corriente Real: "); Serial.print(current, 2); Serial.println(" A");
    Serial.print("Potencia Real: "); Serial.print(power, 2); Serial.println(" W");
    Serial.println("--------------------------------");
    
    // Enviar datos a ThingSpeak
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      WiFiClient client;
      
      String url = String(server) + "?api_key=" + apiKey +
                   "&field1=" + String(voltage, 2) +
                   "&field2=" + String(current, 2) +
                   "&field3=" + String(power, 2) +
                   "&field4=" + String(servov) +
                   "&field5=" + String(servoh);
      http.begin(client, url);
      int httpResponseCode = http.GET();
      if (httpResponseCode > 0) {
        Serial.print("Datos enviados a ThingSpeak, código: ");
        Serial.println(httpResponseCode);
      } else {
        Serial.print("Error en la conexión a ThingSpeak: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    }
    lastPrintTime = currentMillis;
  }

  // Control del servo vertical (inclinación del panel)
  if (abs(dvert) > tol) {
    servov = constrain(servov + (dvert > 0 ? -1 : 1), 0, 140);
    vertical.write(servov);
  }
  
  // Control del servo horizontal (rotación de la base)
  if (abs(dhoriz) > tol) {
    servoh = constrain(servoh + (dhoriz > 0 ? -1 : 1), 0, 140);
    horizontal.write(servoh);
  }
  
  delay(50); // Pequeña pausa para estabilidad
}
