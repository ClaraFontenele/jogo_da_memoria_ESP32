// Jogo de memória sonoro (Color Memory) para ESP32 - 4 cores + buzzer + LCD 16x2 com I2C
// SEM biblioteca de LCD: o código fala direto com o módulo I2C (nada para instalar).
//
// Como o jogo funciona:
//   1) "Aperte qualquer tecla p/ comecar" (LEDs piscando em sequência enquanto espera)
//   2) Nivel 1/7 ... Nivel 7/7: o jogo mostra a sequência (Observe...), você repete (Sua vez!)
//   3) Acertou os 7 níveis: PARABENS! por 5 segundos, com animação e musiquinha
//   4) Volta para "Aperte qualquer tecla p/ comecar"
//   Errou: mostra "Errou!" e também volta para o começo.
//
// Placa na IDE: ESP32 Dev Module | Monitor Serial: 115200 baud
//
// Ligações:
//   LEDs: vermelho 33, amarelo 27, azul 12, branco 18 (cada um com o seu resistor)
//   Botões: vermelho 32, amarelo 25, azul 14, preto 19 (pull-up interno, outro lado no GND)
//   Buzzer passivo: perna longa (+) no GPIO 13, perna curta (-) no GND
//   LCD I2C: GND -> GND, VCC -> VIN (5 V), SDA -> GPIO 21, SCL -> GPIO 22

#include <Wire.h>
#include <esp_system.h>   // para esp_random()

const int NUM_CORES = 4;
const int MAX_NIVEL = 7;

// ===== Pinos =====
const int NUM_LEDS = 4;
const int leds[]   = {33, 27, 12, 18};   // vermelho, amarelo, azul, branco
const int botoes[] = {32, 25, 14, 19};   // vermelho, amarelo, azul, preto
const int buzzer   = 13;
const int SDA_PIN  = 21;
const int SCL_PIN  = 22;

// Notas de cada cor (índice 1 a 4)
const int notas[] = {0, 523, 659, 784, 1047};

// ===== Estado do jogo =====
int sequence[MAX_NIVEL];
int level = 0;

// =====================================================================
//  LCD 16x2 via módulo I2C (PCF8574), sem biblioteca
// =====================================================================
byte ADDR = 0x27;           // o código tenta 0x27 e depois 0x3F
const byte BL = 0x08;       // luz de fundo
const byte EN = 0x04;       // enable
const byte RS = 0x01;       // 0 = comando, 1 = caractere

void expWrite(byte d) {
  Wire.beginTransmission(ADDR);
  Wire.write(d | BL);
  Wire.endTransmission();
}

void pulso(byte d) {
  expWrite(d | EN);
  delayMicroseconds(2);
  expWrite(d & ~EN);
  delayMicroseconds(60);
}

void write4(byte nibble, byte rs) {
  byte d = (nibble << 4) | rs;
  expWrite(d);
  pulso(d);
}

void enviar(byte v, byte rs) {
  write4(v >> 4, rs);
  write4(v & 0x0F, rs);
}

void comando(byte c) {
  enviar(c, 0);
  if (c <= 0x03) delay(3);   // limpar/home demoram mais
}

void posicao(byte coluna, byte linhaLCD) {
  comando(0x80 | (coluna + (linhaLCD ? 0x40 : 0x00)));
}

void texto(const char* t) {
  while (*t) enviar(*t++, RS);
}

// Escreve uma linha inteira (completa com espaços até 16 letras, apagando o que havia antes)
void linha(byte numero, const char* t) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", t);
  posicao(0, numero);
  texto(buf);
}

void mostrar(const char* linha1, const char* linha2) {
  linha(0, linha1);
  linha(1, linha2);
}

void iniciarLCD() {
  // Descobre o endereço do módulo
  byte candidatos[] = {0x27, 0x3F};
  for (byte i = 0; i < 2; i++) {
    Wire.beginTransmission(candidatos[i]);
    if (Wire.endTransmission() == 0) {
      ADDR = candidatos[i];
      break;
    }
  }
  Serial.print("LCD no endereco 0x");
  Serial.println(ADDR, HEX);

  delay(60);
  expWrite(0);
  delay(10);
  // Sequência de inicialização em 4 bits do HD44780
  write4(0x3, 0); delay(6);
  write4(0x3, 0); delay(6);
  write4(0x3, 0); delayMicroseconds(200);
  write4(0x2, 0); delayMicroseconds(200);   // passa para 4 bits
  comando(0x28);   // 4 bits, 2 linhas
  comando(0x0C);   // tela ligada, sem cursor
  comando(0x01);   // limpa
  comando(0x06);   // cursor anda para a direita
}

// =====================================================================
//  Funções do jogo
// =====================================================================
void allLEDs(bool on) {
  for (int i = 0; i < NUM_LEDS; i++) digitalWrite(leds[i], on ? HIGH : LOW);
}

bool anyButtonPressed() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (digitalRead(botoes[i]) == LOW) return true;
  }
  return false;
}

// Acende o LED da cor (1 a 4) e toca a nota dela
void lightUpLED(int color, int tempo) {
  digitalWrite(leds[color - 1], HIGH);
  tone(buzzer, notas[color], tempo);
  delay(tempo);
  allLEDs(false);
}

// Tela inicial: espera qualquer botão, com os LEDs piscando em sequência
void esperarTecla() {
  mostrar("Aperte qualquer", "tecla p/ comecar");

  unsigned long ultimo = 0;
  int p = 0;
  while (!anyButtonPressed()) {
    if (millis() - ultimo > 250) {
      allLEDs(false);
      digitalWrite(leds[p], HIGH);
      p = (p + 1) % NUM_LEDS;
      ultimo = millis();
    }
    delay(5);
  }
  allLEDs(false);

  randomSeed(micros() ^ (uint32_t)esp_random());   // sorteio diferente a cada jogo

  tone(buzzer, 1000, 200);
  while (anyButtonPressed()) delay(5);   // espera soltar o botão
  delay(300);
}

// Mostra "Sua vez!" e o progresso: '.' = falta apertar, '*' = já acertou
void telaSuaVez(int feitos) {
  char l2[17];
  int n = snprintf(l2, sizeof(l2), "Sua vez! ");
  for (int j = 0; j < level && n < 16; j++) {
    l2[n++] = (j < feitos) ? '*' : '.';
  }
  l2[n] = 0;
  linha(1, l2);
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

  delay(400);
  for (int i = 0; i < level; i++) {
    lightUpLED(sequence[i], tempo);
    delay(tempo / 2);
  }
}

bool getPlayerInput() {
  char buf[20];
  snprintf(buf, sizeof(buf), "Nivel %d/%d", level, MAX_NIVEL);
  linha(0, buf);
  telaSuaVez(0);

  for (int i = 0; i < level; i++) {
    int apertado = 0;

    while (apertado == 0) {
      for (int c = 0; c < NUM_CORES; c++) {
        if (digitalRead(botoes[c]) == LOW) {
          apertado = c + 1;
          break;
        }
      }
      delay(5);
    }

    lightUpLED(apertado, 300);

    if (apertado != sequence[i]) {
      return false;
    }

    telaSuaVez(i + 1);

    while (anyButtonPressed()) delay(5);
    delay(200);
  }

  level++;
  return true;
}

void endGame() {
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
  delay(1000);
}

// PARABENS por 5 segundos: estrelas passando na tela, LEDs correndo e a musiquinha
void winGame() {
  Serial.println("Voce venceu! Parabens!");
  linha(0, "   PARABENS!");

  const int melodia[] = {523, 659, 784, 1047, 784, 659, 523, 784, 1047, 1319, 1047, 784};
  const int tamanho = sizeof(melodia) / sizeof(melodia[0]);

  unsigned long inicio = millis();
  int passo = 0;
  while (millis() - inicio < 5000) {
    char l2[17];
    for (int c = 0; c < 16; c++) {
      l2[c] = ((c + passo) % 4 == 0) ? '*' : ' ';
    }
    l2[16] = 0;
    posicao(0, 1);
    texto(l2);

    allLEDs(false);
    digitalWrite(leds[passo % NUM_LEDS], HIGH);
    tone(buzzer, melodia[passo % tamanho], 150);

    delay(200);
    passo++;
  }

  noTone(buzzer);
  allLEDs(false);
}

// Um jogo completo, do nível 1 até vencer ou errar
void jogar() {
  level = 1;
  while (true) {
    playSequence();

    if (!getPlayerInput()) {
      endGame();
      return;
    }
    if (level > MAX_NIVEL) {
      winGame();
      return;
    }
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
    pinMode(botoes[i], INPUT_PULLUP);
  }
  pinMode(buzzer, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  iniciarLCD();

  Serial.println("Jogo pronto.");
}

void loop() {
  esperarTecla();
  jogar();
}
