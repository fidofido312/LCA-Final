#define PIN_TACTIL T0       // Pin táctil
const int PIN_LED = 22;     // Pin del LED
const int UMBRAL_VALOR = 30; // Valor límite para activar el LED

int valorSensorTactil;

void setup() {
  delay(1000); 
  pinMode(PIN_LED, OUTPUT);   // Configura el LED como salida
  Serial.begin(9600);         // Habilita el monitor serial
}

void loop() {
  valorSensorTactil = touchRead(PIN_TACTIL);  // Lee el valor del sensor táctil
  Serial.print("Valor tactil: ");
  Serial.println(valorSensorTactil);

  if (valorSensorTactil < UMBRAL_VALOR) {
    digitalWrite(PIN_LED, HIGH);  // Enciende el LED
  } else {
    digitalWrite(PIN_LED, LOW);   // Apaga el LED
  }
}
