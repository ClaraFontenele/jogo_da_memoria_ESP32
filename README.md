Jogo da Memória Sonoro (Genius / Simon) com ESP32
Jogo de memória com 3 cores (LEDs + botões), som no buzzer e tela OLED.
O ESP32 sorteia uma sequência de cores e sons; você repete apertando os botões na mesma ordem.
A cada nível a sequência cresce em 1 cor e fica mais rápida. Quem completar o nível 7 vence.
Inspirado no projeto "Color Memory" da SunFounder.
Arquivo do código: `jogo_memoria_esp32_oled_final.ino`
---
O que o jogo faz
Ao ligar: todos os LEDs acendem e o buzzer toca um bipe; a tela mostra "Jogo da Memoria".
Em cada nível, a tela mostra "Nivel X/7 - Observe..." enquanto os LEDs piscam a sequência com um som para cada cor.
Depois mostra "Nivel X/7 - Sua vez!" e você repete a sequência com os botões.
Cores e notas: vermelho = 523 Hz, amarelo = 659 Hz, verde = 784 Hz.
Errou: a tela mostra "Errou!", os LEDs piscam 5 vezes com som grave e um novo jogo começa.
Completou o nível 7: a tela mostra "VENCEU! Parabens!", toca uma melodia de vitória e o jogo reinicia.
A velocidade aumenta a cada nível (300 ms no nível 1 até 150 ms no nível 7).
Se o OLED não for encontrado, o jogo continua funcionando sem a tela.
O Monitor Serial (115200 baud) mostra o andamento do jogo.
---
Materiais
1 placa ESP32 (ESP32 Dev Module, 30 pinos)
3 LEDs (vermelho, amarelo, verde) + 3 resistores de 220 ohms
3 botões (push button de 4 pernas)
1 buzzer passivo (o ativo toca um tom só e não serve)
1 display OLED 0,96" I2C (SSD1306, 128x64)
Protoboard e fios jumper
---
Tabela de ligações
Lado esquerdo do ESP32
Peça	Pino
LED vermelho	27
LED amarelo	14
LED verde	12
Botão vermelho	32
Botão amarelo	33
Botão verde	25
Buzzer (+)	13
Pinos livres neste lado: 26 / 35 / 34 / VP / VN
Lado direito do ESP32
OLED	Pino do ESP32
SDA	21
SCK (SCL)	22
VDD (VCC)	3V3
GND	GND
Pinos livres neste lado: 23 / 19 / 18 / 5 / 4 / 17 (TX2) / 16 (RX2)
Evitar: TX0 e RX0 (são a porta USB) e D2 e D15 (pinos de boot).
Como ligar cada peça
LED: perna longa (+) no pino do ESP32 passando pelo resistor de 220R; perna curta (-) no GND.
Botão: um lado no pino do ESP32 e o outro no GND. O botão fica atravessando o vão central da protoboard. O código usa o pull-up interno, então não precisa de resistor no botão.
Buzzer passivo: perna longa (+) no pino 13; perna curta (-) no GND.
OLED: os 4 fios nos pinos do lado direito conforme a tabela acima.
---
O que instalar
1. Arduino IDE
Versão 2.x. Baixe em https://www.arduino.cc/en/software
2. Suporte à placa ESP32
Abra File > Preferences.
Em Additional boards manager URLs coloque:
`https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
Abra Tools > Board > Boards Manager, procure esp32 (autor Espressif Systems) e instale.
Ocupa vários GB; deixe uns 5 GB livres no disco.
3. Bibliotecas
Em Tools > Manage Libraries (ou Sketch > Include Library > Manage Libraries), instale:
Biblioteca	Autor	Para que serve
Adafruit SSD1306	Adafruit	controla o display OLED
Adafruit GFX Library	Adafruit	desenha texto e gráficos (dependência do SSD1306)
Se a IDE perguntar "Install all" (instalar dependências), clique em Install all.
As demais bibliotecas usadas (`Wire`, `esp_system`) já vêm com o pacote do ESP32.
4. Configuração na IDE
Tools > Board > esp32 > ESP32 Dev Module (não escolha "Arduino Nano ESP32")
Tools > Port: a porta COM do ESP32 (ex.: COM5)
Monitor Serial: 115200 baud
---
Como gravar
Conecte o ESP32 ao computador pelo cabo USB (use um cabo que transmita dados).
Abra o arquivo `jogo_memoria_esp32_oled_final.ino` na IDE.
Confira a placa e a porta.
Clique na seta de Upload (não no botão de debug).
Quando terminar, a tela mostra "Jogo da Memoria" e o jogo começa.
---
Dicas e solução de problemas
Se a tela não acender
Confira se o fio do VDD está no pino 3V3, e não no VIN.
Confira se o SDA está no 21 e o SCK no 22, e não trocados.
Se mesmo assim nada aparecer, a tela pode ter outro endereço I2C (o código usa `0x3C`). Troque `0x3C` por `0x3D` na linha `display.begin(...)` e grave de novo. Se ainda não funcionar, rode um sketch "I2C Scanner" para descobrir o endereço.
Confira se os fios estão bem encaixados nos pinos do OLED.
Se o upload falhar
"Could not open COM5 / Acesso negado": feche o Monitor Serial e qualquer outra janela que use a porta, e reconecte o cabo.
"No upload port" ou porta errada: escolha a porta correta em Tools > Port.
Falha no upload com o LED verde ligado: o pino 12 é um pino de boot. Solte o fio do LED verde, grave, e depois ligue de novo.
Erro `unlinkat` ao compilar: apague o conteúdo da pasta `AppData\Local\arduino\sketches` e tente de novo.
Falta de espaço ao instalar o pacote ESP32: libere espaço no disco (cerca de 5 GB) e instale de novo.
Se o jogo se comporta de forma estranha
"Errou!" logo no começo, sem apertar nada: o ESP32 pode estar frouxo na protoboard. Encaixe bem a placa. Confira também se cada botão tem um lado no pino e o outro no GND.
Um botão não responde: teste o botão separadamente com um sketch simples que leia o pino e imprima no Monitor Serial.
O buzzer não toca: confirme que é o passivo e que a perna longa (+) está no pino 13.
Buzzer fraco: o buzzer passivo é mais alto em frequências agudas. As notas do código já são agudas; se quiser, aumente os valores no vetor `notas`.
Regras que ajudam na protoboard
Uma coluna de 5 furos é ligada entre si (de um lado do vão central).
Os botões ficam atravessando o vão central.
Em LED, a perna longa é o positivo (anodo).
Nunca ligue o VIN (5 V) em um pino do ESP32.
---
Personalizar
No começo do arquivo `.ino`:
`MAX_NIVEL` - quantos níveis o jogo tem (padrão: 7).
`notas[]` - frequência (Hz) de cada cor.
Velocidade: a linha `int tempo = 300 - (level - 1) * 25;` controla o quanto a sequência acelera.
Os pinos estão nas constantes `redLED`, `redButton`, `buzzer`, `SDA_PIN` e `SCL_PIN`.
