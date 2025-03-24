// Pin yang akan dipantau
// const int monitoredPin = 5; // Pin yang akan membaca sinyal
// const int actionPin = 4;  // Pin aksi (misalnya LED)
const int monitoredPin = 3; // Pin yang akan membaca sinyal
const int actionPin = 4;  // Pin aksi (misalnya LED)
const int led = 0;

// Variabel untuk menyimpan status terakhir
int lastState = LOW;

int counter = 0;

void setup() {
  pinMode(monitoredPin, INPUT);  // Atur pin sebagai input
  pinMode(actionPin, OUTPUT);   // Atur pin aksi sebagai output
  pinMode(led, OUTPUT);   // Atur pin aksi sebagai output
  // digitalWrite(actionPin, LOW); // Pastikan aksi dalam kondisi mati awalnya

  // delay(2000);

  lastState = digitalRead(monitoredPin); // Baca status awal
  digitalWrite(actionPin, HIGH);
}

void loop() {
  int currentState = digitalRead(monitoredPin); // Baca status pin

  // Jika ada perubahan status
  if (currentState != lastState) {
    lastState = currentState;
    counter = 0;
    digitalWrite(led, lastState);
  }

  if(counter >= 1800) {
    performAction();
    counter = 0;
  }

  counter++;
  delay(100);
}

void performAction() {
  // Aksi yang dilakukan jika tidak ada perubahan
  digitalWrite(actionPin, LOW);  // Nyalakan LED sebagai tanda
  delay(3000);                    // Tunda 3 detik
  digitalWrite(actionPin, HIGH);   // Matikan LED
}
