
const int RPWM = 3;    // Пин для управления скоростью (ШИМ)
const int R_EN = 4;    // Пин разрешения (Enable)


void setup() {
  Serial.begin(115200);

  pinMode(RPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);

  // Активируем драйвер, подав HIGH на пины EN
  digitalWrite(R_EN, HIGH);

  // MOSFET
  pinMode(9, OUTPUT); // CTRL
  pinMode(8, OUTPUT); // GND
  digitalWrite(8, LOW);
}

void loop() {
  digitalWrite(9, HIGH);
  delayMicroseconds(50);
  digitalWrite(9, LOW);
  delayMicroseconds(500);

  if (Serial.available() > 0) {
    int val = Serial.parseInt();
    analogWrite(RPWM, val);
    Serial.print("PWM: ");
    Serial.println(val);
  }
}

