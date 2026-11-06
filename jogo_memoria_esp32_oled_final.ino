// Jogo de memória sonoro (Color Memory) para ESP32 - 3 cores + buzzer + OLED
// 7 níveis. A tela mostra o nível e o que fazer (Observe / Sua vez).
//
// Placa na IDE: ESP32 Dev Module | Monitor Serial: 115200 baud
// Bibliotecas (Gerenciador de Bibliotecas): "Adafruit SSD1306" e "Adafruit GFX Library"
//
// Ligações:
//   LEDs: vermelho 27, amarelo 14, verde 12 (cada um com o seu resistor)
//   Botões: vermelho 32, amarelo 33, verde 25 (pull-up interno, outro lado no GND)
//   Buzzer passivo: perna longa (+) no GPIO 13, perna curta (-) no GND
//   OLED (lado direito do ESP32): SDA no 21, SCK/SCL no 22, VDD no 3V3, GND no GND

#include <esp_system.h>   // para esp_random()
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int NUM_CORES = 3;
const int MAX_NIVEL = 7;

// ===== LEDs (saídas) =====
const int redLED    = 27;  // cor 1
const int yellowLED = 14;  // cor 2
const int greenLED  = 12;  // cor 3 (pino de boot: se o upload falhar, solte o fio dele)

// ===== Botões (apertado = LOW, pull-up interno) =====
const int redButton    = 32;
const int yellowButton = 33;
const int greenButton  = 25;

const int buzzer = 13;     // buzzer passivo

// ===== OLED (I2C em pinos escolhidos por nós) =====
const int SDA_PIN = 21;
const int SCL_PIN = 22;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool temTela = false;

// Notas de cada cor (agudas = mais altas no buzzer)
const int notas[] = {0, 523, 659, 784};  // índice 1 a 3

// ===== Estado do jogo =====
int sequence[MAX_NIVEL];
int playerInput[MAX_NIVEL];
int level = 0;
bool gameOver = false;

void startGame();
void playSequence();
bool getPlayerInput();
void endGame();
void winGame();
void lightUpLED(int color, int tempo);
bool anyButtonPressed();
void allLEDs(bool on);
void mostrar(const char* linha1, const char* linha2);

void setup() {
  Serial.begin(115200);

  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);

  pinMode(redButton, INPUT_PULLUP);
  pinMode(yellowButton, INPUT_PULLUP);
  pinMode(greenButton, INPUT_PULLUP);

  pinMode(buzzer, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  temTela = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (temTela) {
    display.dim(true);  // brilho mais baixo (mais suave)
  } else {
    Serial.println("OLED nao encontrado: o jogo segue sem a tela.");
  }

  randomSeed((uint32_t)esp_random());

  startGame();
}

void loop() {
  if (!gameOver) {
    playSequence();

    if (!getPlayerInput()) {
      endGame();
    } else if (level > MAX_NIVEL) {
      winGame();
    } else {
      delay(500);
    }
  } else {
    delay(1500);
    startGame();
  }
}

void mostrar(const char* linha1, const char* linha2) {
  if (!temTela) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 8);
  display.println(linha1);
  display.setCursor(0, 38);
  display.println(linha2);
  display.display();
}

void allLEDs(bool on) {
  digitalWrite(redLED, on ? HIGH : LOW);
  digitalWrite(yellowLED, on ? HIGH : LOW);
  digitalWrite(greenLED, on ? HIGH : LOW);
}

bool anyButtonPressed() {
  return digitalRead(redButton)    == LOW ||
         digitalRead(yellowButton) == LOW ||
         digitalRead(greenButton)  == LOW;
}

void startGame() {
  level = 1;
  gameOver = false;
  Serial.println("Novo jogo!");
  mostrar("Jogo da", "Memoria");

  allLEDs(true);
  tone(buzzer, 1000, 500);
  delay(500);
  allLEDs(false);

  delay(500);
}

void playSequence() {
  sequence[level - 1] = random(1, NUM_CORES + 1);

  char buf[20];
  snprintf(buf, sizeof(buf), "Nivel %d/%d", level, MAX_NIVEL);
  Serial.println(buf);
  mostrar(buf, "Observe...");

  // Fica mais rápido a cada nível: 300 ms no nível 1, 150 ms no nível 7
  int tempo = 300 - (level - 1) * 25;
  if (tempo < 150) tempo = 150;

  for (int i = 0; i < level; i++) {
    lightUpLED(sequence[i], tempo);
    delay(tempo / 2);
  }
}

bool getPlayerInput() {
  char buf[20];
  snprintf(buf, sizeof(buf), "Nivel %d/%d", level, MAX_NIVEL);
  mostrar(buf, "Sua vez!");

  for (int i = 0; i < level; i++) {
    bool inputReceived = false;

    while (!inputReceived) {
      if (digitalRead(redButton) == LOW) {
        playerInput[i] = 1;
        lightUpLED(1, 300);
        inputReceived = true;
      } else if (digitalRead(yellowButton) == LOW) {
        playerInput[i] = 2;
        lightUpLED(2, 300);
        inputReceived = true;
      } else if (digitalRead(greenButton) == LOW) {
        playerInput[i] = 3;
        lightUpLED(3, 300);
        inputReceived = true;
      }
      delay(5);
    }

    if (playerInput[i] != sequence[i]) {
      return false;
    }

    while (anyButtonPressed()) {
      delay(5);
    }

    delay(200);
  }

  level++;
  return true;
}

void endGame() {
  gameOver = true;

  char buf[20];
  snprintf(buf, sizeof(buf), "Nivel %d", level);
  Serial.print("Errou no ");
  Serial.println(buf);
  mostrar("Errou!", buf);

  for (int i = 0; i < 5; i++) {
    allLEDs(true);
    tone(buzzer, 400, 300);
    delay(200);
    allLEDs(false);
    delay(200);
  }
}

void winGame() {
  gameOver = true;
  Serial.println("Voce venceu! Parabens!");
  mostrar("VENCEU!", "Parabens!");

  const int melodia[] = {523, 659, 784, 1047};
  const int leds[] = {redLED, yellowLED, greenLED, redLED};

  delay(300);
  for (int vez = 0; vez < 2; vez++) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(leds[i], HIGH);
      tone(buzzer, melodia[i], 150);
      delay(180);
      digitalWrite(leds[i], LOW);
    }
  }

  allLEDs(true);
  tone(buzzer, 1047, 600);
  delay(700);
  allLEDs(false);
}

void lightUpLED(int color, int tempo) {
  switch (color) {
    case 1: digitalWrite(redLED, HIGH); break;
    case 2: digitalWrite(yellowLED, HIGH); break;
    case 3: digitalWrite(greenLED, HIGH); break;
  }
  tone(buzzer, notas[color], tempo);

  delay(tempo);
  allLEDs(false);
}
