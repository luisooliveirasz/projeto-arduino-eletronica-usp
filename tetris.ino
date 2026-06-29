/*
  ===========================================================================
  TETRIS - Arduino Nano + 2x matriz 8x8 MAX7219 (cascateadas em 8x16) + botões + buzzer
  ===========================================================================

  HARDWARE / LIGAÇÕES:
  ---------------------------------------------------------------------------
  Matriz MAX7219 (cascateada - DOUT da matriz 1 -> DIN da matriz 2):
    VCC -> 5V do Arduino
    GND -> GND do Arduino
    DIN -> D12
    CS  -> D10
    CLK -> D11

  Botões (cada um liga o pino ao GND; usamos INPUT_PULLUP, não precisa resistor):
    D2 -> Botão ESQUERDA
    D3 -> Botão DIREITA
    D4 -> Botão GIRAR
    D5 -> Botão DESCER RÁPIDO (soft drop)
    D6 -> Botão PAUSE / START

  Buzzer passivo 5V:
    Pino positivo -> D8
    Pino negativo -> GND

  BIBLIOTECA NECESSÁRIA:
  ---------------------------------------------------------------------------
  - "LedControl" de Eberhard Fahle)

  COMO JOGAR:
  ---------------------------------------------------------------------------
  - Ao ligar, a tela mostra uma animação de início. Aperte PAUSE/START para começar.
  - Esquerda/Direita movem a peça.
  - Girar roda a peça 90°.
  - Descer rápido acelera a queda enquanto pressionado.
  - Pause/Start pausa e despausa o jogo a qualquer momento.
  - Pontuação: 40/100/300/1200 pontos x nível, igual ao Tetris clássico de Game Boy.
    A pontuação aparece no Monitor Serial (9600 baud) em tempo real, já que a
    matriz de LED só tem espaço físico para o campo de jogo.
  - Quando empilhar até o topo, o jogo mostra animação de Game Over, toca som
    de derrota e reinicia automaticamente após alguns segundos.

  ===========================================================================
*/

#include <LedControl.h>

// ---------------------------------------------------------------------------
// CONFIGURAÇÃO DE PINOS
// ---------------------------------------------------------------------------
#define DIN_PIN   12
#define CS_PIN    10
#define CLK_PIN   11

#define BTN_LEFT   2
#define BTN_RIGHT  3
#define BTN_ROTATE 4
#define BTN_DOWN   5
#define BTN_PAUSE  6

#define BUZZER_PIN 8

// Dois módulos MAX7219 cascateados. addr 0 = parte de CIMA do campo,
// addr 1 = parte de BAIXO do campo (linhas 0-7 e 8-15).
// Se ao testar a matriz aparecer invertida (de cabeça para baixo ou trocada),
// troque o valor de TOPO_ADDR e FUNDO_ADDR abaixo (0 <-> 1).
#define TOPO_ADDR  0
#define FUNDO_ADDR 1

LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, 2); // 2 dispositivos cascateados

// ---------------------------------------------------------------------------
// CAMPO DE JOGO: 8 colunas x 16 linhas
// ---------------------------------------------------------------------------
#define LARGURA 8
#define ALTURA  16

// campo[linha][coluna] = 0 (vazio) ou 1 (ocupado)
byte campo[ALTURA][LARGURA];

// ---------------------------------------------------------------------------
// PEÇAS (TETROMINÓS) - cada peça tem 4 rotações, cada rotação é uma grade 4x4
// 1 = bloco preenchido, 0 = vazio
// ---------------------------------------------------------------------------
const byte PECAS[7][4][4][4] = {
  // I
  {
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
    {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
    {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}
  },
  // O
  {
    {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}
  },
  // T
  {
    {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
    {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}},
    {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}}
  },
  // S
  {
    {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
    {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}}
  },
  // Z
  {
    {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
    {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}}
  },
  // J
  {
    {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}},
    {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
    {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}}
  },
  // L
  {
    {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
    {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
    {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}
  }
};

// ---------------------------------------------------------------------------
// ESTADO DO JOGO
// ---------------------------------------------------------------------------
int pecaAtual;       // índice 0-6
int rotacaoAtual;    // 0-3
int pecaX, pecaY;    // posição da peça (canto superior esquerdo do grid 4x4)
int proximaPeca;

unsigned long pontuacao = 0;
int nivel = 0;
int linhasTotais = 0;

bool jogoPausado = true;
bool jogoAtivo = false;     // false = tela de início ou game over
bool gameOver = false;

unsigned long ultimaQueda = 0;
unsigned long intervaloQueda = 800; // ms entre quedas automáticas, diminui com o nível

// ---------------------------------------------------------------------------
// DEBOUNCE DE BOTÕES
// ---------------------------------------------------------------------------
struct Botao {
  byte pino;
  bool estadoAtual;
  bool estadoAnterior;
  unsigned long ultimoTrigger;
};

Botao btnLeft   = {BTN_LEFT, HIGH, HIGH, 0};
Botao btnRight  = {BTN_RIGHT, HIGH, HIGH, 0};
Botao btnRotate = {BTN_ROTATE, HIGH, HIGH, 0};
Botao btnDown   = {BTN_DOWN, HIGH, HIGH, 0};
Botao btnPause  = {BTN_PAUSE, HIGH, HIGH, 0};

#define DEBOUNCE_MS 150
#define REPEAT_MS   120  // repetição ao manter pressionado (esquerda/direita/descer)

// retorna true se o botão acabou de ser pressionado (transição HIGH->LOW), com debounce
bool botaoPressionado(Botao &b) {
  bool leitura = digitalRead(b.pino);
  bool pressionadoAgora = false;
  if (leitura == LOW && b.estadoAnterior == HIGH) {
    if (millis() - b.ultimoTrigger > DEBOUNCE_MS) {
      pressionadoAgora = true;
      b.ultimoTrigger = millis();
    }
  }
  b.estadoAnterior = leitura;
  return pressionadoAgora;
}

// retorna true se o botão está sendo mantido pressionado e já passou o intervalo de repetição
bool botaoRepetindo(Botao &b) {
  bool leitura = digitalRead(b.pino);
  if (leitura == LOW) {
    if (millis() - b.ultimoTrigger > REPEAT_MS) {
      b.ultimoTrigger = millis();
      return true;
    }
  } else {
    b.estadoAnterior = HIGH;
  }
  return false;
}

// ---------------------------------------------------------------------------
// SOM - tocador não-bloqueante (toca a música em paralelo com o jogo)
// ---------------------------------------------------------------------------
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_E1  41
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define REST     0

// Música ("Korobeiniki")
const int melodia[] PROGMEM = {
  NOTE_E5, 4,  NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4,  NOTE_C5, 8,  NOTE_B4, 8,
  NOTE_A4, 4,  NOTE_A4, 8,  NOTE_C5, 8,  NOTE_E5, 4,  NOTE_D5, 8,  NOTE_C5, 8,
  NOTE_B4, -4,           NOTE_C5, 8,  NOTE_D5, 4,  NOTE_E5, 4,
  NOTE_C5, 4,  NOTE_A4, 4,  NOTE_A4, 8,  REST,    8,  REST,    4,

  NOTE_D5, 4,  NOTE_F5, 8,  NOTE_A5, 4,  NOTE_G5, 8,  NOTE_F5, 8,
  NOTE_E5, -4,           NOTE_C5, 8,  NOTE_E5, 4,  NOTE_D5, 8,  NOTE_C5, 8,
  NOTE_B4, 4,  NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4,  NOTE_E5, 4,
  NOTE_C5, 4,  NOTE_A4, 4,  NOTE_A4, 4,  REST,    4,

  NOTE_E5, 4,  NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4,  NOTE_C5, 8,  NOTE_B4, 8,
  NOTE_A4, 4,  NOTE_A4, 8,  NOTE_C5, 8,  NOTE_E5, 4,  NOTE_D5, 8,  NOTE_C5, 8,
  NOTE_B4, -4,           NOTE_C5, 8,  NOTE_D5, 4,  NOTE_E5, 4,
  NOTE_C5, 4,  NOTE_A4, 4,  NOTE_A4, 8,  REST,    8,  REST,    4,

  NOTE_D5, 4,  NOTE_F5, 8,  NOTE_A5, 4,  NOTE_G5, 8,  NOTE_F5, 8,
  NOTE_E5, -4,           NOTE_C5, 8,  NOTE_E5, 4,  NOTE_D5, 8,  NOTE_C5, 8,
  NOTE_B4, 4,  NOTE_B4, 8,  NOTE_C5, 8,  NOTE_D5, 4,  NOTE_E5, 4,
  NOTE_C5, 4,  NOTE_A4, 4,  NOTE_A4, 4,  REST,    4,
};
const int totalNotasMelodia = sizeof(melodia) / sizeof(melodia[0]) / 2;

int musTempo = 144;
long musWholeNote = (60000L * 4) / musTempo;

int musIndiceNota = 0;
unsigned long musProximoEvento = 0;
bool musTocandoNota = false;
bool musicaAtiva = true; // toca em loop enquanto o jogo está rodando

void atualizarMusica() {
  if (!musicaAtiva) return;
  unsigned long agora = millis();
  if (agora < musProximoEvento) return;

  if (musTocandoNota) {
    // termina a nota atual (deixa um pequeno silêncio entre notas)
    noTone(BUZZER_PIN);
    musTocandoNota = false;
    musProximoEvento = agora; // já pode seguir pra próxima nota imediatamente
    return;
  }

  // toca a próxima nota
  int freq = pgm_read_word(&melodia[musIndiceNota * 2]);
  int divider = pgm_read_word(&melodia[musIndiceNota * 2 + 1]);
  // pgm_read_word lê unsigned, então valores negativos (pontuados) precisam de cast
  int16_t divSigned = (int16_t)divider;

  long duracao;
  if (divSigned > 0) {
    duracao = musWholeNote / divSigned;
  } else {
    duracao = musWholeNote / (-divSigned);
    duracao = duracao * 3 / 2;
  }

  if (freq != REST) {
    tone(BUZZER_PIN, freq);
  } else {
    noTone(BUZZER_PIN);
  }

  musTocandoNota = true;
  musProximoEvento = agora + (duracao * 9 / 10); // 90% tocando, 10% silêncio (tratado na próxima chamada)

  musIndiceNota++;
  if (musIndiceNota >= totalNotasMelodia) {
    musIndiceNota = 0;
  }
}

// Efeitos sonoros simples
void somRotacao() {
  tone(BUZZER_PIN, 880, 30);
}

void somQueda() {
  tone(BUZZER_PIN, 220, 40);
}

void somLinhaCompleta(int numLinhas) {
  noTone(BUZZER_PIN);
  for (int i = 0; i < numLinhas + 1; i++) {
    tone(BUZZER_PIN, 1200, 60);
    delay(70);
    tone(BUZZER_PIN, 1600, 60);
    delay(70);
  }
  noTone(BUZZER_PIN);
}

void somGameOver() {
  musicaAtiva = false;
  noTone(BUZZER_PIN);
  tone(BUZZER_PIN, 523, 200); delay(220);
  tone(BUZZER_PIN, 466, 200); delay(220);
  tone(BUZZER_PIN, 440, 200); delay(220);
  tone(BUZZER_PIN, 392, 400); delay(420);
  noTone(BUZZER_PIN);
}

// ---------------------------------------------------------------------------
// FUNÇÕES DA MATRIZ DE LED
// ---------------------------------------------------------------------------

// Liga/desliga um pixel lógico do campo (x: 0-7, y: 0-15)
// y 0-7 vai para o módulo TOPO_ADDR, y 8-15 vai para o módulo FUNDO_ADDR
void setPixel(int x, int y, bool estado) {
  if (x < 0 || x >= LARGURA || y < 0 || y >= ALTURA) return;
  if (y < 8) {
    lc.setLed(TOPO_ADDR, y, x, estado);
  } else {
    lc.setLed(FUNDO_ADDR, y - 8, x, estado);
  }
}

// Redesenha a matriz inteira a partir do array "campo" + peça atual em queda
void desenharCampo() {
  // monta um buffer temporário com campo fixo + peça atual, depois envia tudo de uma vez
  static byte buffer[ALTURA][LARGURA];
  for (int y = 0; y < ALTURA; y++) {
    for (int x = 0; x < LARGURA; x++) {
      buffer[y][x] = campo[y][x];
    }
  }
  // adiciona a peça em queda ao buffer (se o jogo estiver ativo)
  if (jogoAtivo && !gameOver) {
    for (int ly = 0; ly < 4; ly++) {
      for (int lx = 0; lx < 4; lx++) {
        if (PECAS[pecaAtual][rotacaoAtual][ly][lx]) {
          int gx = pecaX + lx;
          int gy = pecaY + ly;
          if (gx >= 0 && gx < LARGURA && gy >= 0 && gy < ALTURA) {
            buffer[gy][gx] = 1;
          }
        }
      }
    }
  }
  // envia para a matriz física
  for (int y = 0; y < ALTURA; y++) {
    for (int x = 0; x < LARGURA; x++) {
      setPixel(x, y, buffer[y][x]);
    }
  }
}

void limparMatriz() {
  lc.clearDisplay(TOPO_ADDR);
  lc.clearDisplay(FUNDO_ADDR);
}

// ---------------------------------------------------------------------------
// LÓGICA DO TETRIS
// ---------------------------------------------------------------------------

bool colisao(int peca, int rot, int novoX, int novoY) {
  for (int ly = 0; ly < 4; ly++) {
    for (int lx = 0; lx < 4; lx++) {
      if (PECAS[peca][rot][ly][lx]) {
        int gx = novoX + lx;
        int gy = novoY + ly;
        // fora dos limites laterais ou abaixo do fundo = colisão
        if (gx < 0 || gx >= LARGURA || gy >= ALTURA) return true;
        // acima do topo (ainda entrando na tela) é permitido, só ignora checagem do campo
        if (gy >= 0 && campo[gy][gx]) return true;
      }
    }
  }
  return false;
}

int novaPecaAleatoria() {
  return random(0, 7);
}

void spawnPeca() {
  pecaAtual = proximaPeca;
  proximaPeca = novaPecaAleatoria();
  rotacaoAtual = 0;
  pecaX = 2; // centraliza a grade 4x4 numa largura de 8
  pecaY = -1; // começa um pouco acima do topo visível para spawn suave

  if (colisao(pecaAtual, rotacaoAtual, pecaX, pecaY)) {
    // não há espaço para a nova peça -> fim de jogo
    gameOver = true;
  }
}

void fixarPeca() {
  for (int ly = 0; ly < 4; ly++) {
    for (int lx = 0; lx < 4; lx++) {
      if (PECAS[pecaAtual][rotacaoAtual][ly][lx]) {
        int gx = pecaX + lx;
        int gy = pecaY + ly;
        if (gy >= 0 && gy < ALTURA && gx >= 0 && gx < LARGURA) {
          campo[gy][gx] = 1;
        }
      }
    }
  }
}

// Verifica e remove linhas completas. Retorna quantas linhas foram removidas.
int limparLinhasCompletas() {
  int linhasRemovidas = 0;
  for (int y = ALTURA - 1; y >= 0; y--) {
    bool completa = true;
    for (int x = 0; x < LARGURA; x++) {
      if (!campo[y][x]) { completa = false; break; }
    }
    if (completa) {
      linhasRemovidas++;
      // desce todas as linhas de cima uma posição
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < LARGURA; x++) {
          campo[yy][x] = campo[yy - 1][x];
        }
      }
      // limpa a linha do topo
      for (int x = 0; x < LARGURA; x++) campo[0][x] = 0;
      y++; // reavalia a mesma posição de linha (agora com conteúdo da linha de cima)
    }
  }
  return linhasRemovidas;
}

// Animação simples: pisca as linhas completas antes de removê-las
void animarLinhasCompletas() {
  for (int y = 0; y < ALTURA; y++) {
    bool completa = true;
    for (int x = 0; x < LARGURA; x++) {
      if (!campo[y][x]) { completa = false; break; }
    }
    if (completa) {
      for (int flash = 0; flash < 3; flash++) {
        for (int x = 0; x < LARGURA; x++) setPixel(x, y, false);
        delay(60);
        for (int x = 0; x < LARGURA; x++) setPixel(x, y, true);
        delay(60);
      }
    }
  }
}

// Pontuação clássica de Game Boy: 40/100/300/1200 * (nivel+1)
void adicionarPontuacao(int linhas) {
  unsigned long pontosBase = 0;
  switch (linhas) {
    case 1: pontosBase = 40; break;
    case 2: pontosBase = 100; break;
    case 3: pontosBase = 300; break;
    case 4: pontosBase = 1200; break;
    default: pontosBase = 0; break;
  }
  pontuacao += pontosBase * (nivel + 1);
  linhasTotais += linhas;

  // a cada 10 linhas, sobe de nível e acelera a queda
  int novoNivel = linhasTotais / 10;
  if (novoNivel != nivel) {
    nivel = novoNivel;
    intervaloQueda = max(100, 800 - (nivel * 60));
  }

  Serial.print(F("Linhas: "));
  Serial.print(linhas);
  Serial.print(F(" | Pontuacao: "));
  Serial.print(pontuacao);
  Serial.print(F(" | Nivel: "));
  Serial.print(nivel);
  Serial.print(F(" | Total linhas: "));
  Serial.println(linhasTotais);
}

void travarPecaEAvaliar() {
  fixarPeca();
  somQueda();

  // checa se há linhas completas antes de remover, para animar
  bool temLinhaCompleta = false;
  for (int y = 0; y < ALTURA; y++) {
    bool completa = true;
    for (int x = 0; x < LARGURA; x++) {
      if (!campo[y][x]) { completa = false; break; }
    }
    if (completa) { temLinhaCompleta = true; break; }
  }

  if (temLinhaCompleta) {
    desenharCampo(); // garante que a matriz mostre o estado atual antes da animação
    animarLinhasCompletas();
    int linhas = limparLinhasCompletas();
    somLinhaCompleta(linhas);
    adicionarPontuacao(linhas);
  }

  spawnPeca();
}

void resetarJogo() {
  for (int y = 0; y < ALTURA; y++)
    for (int x = 0; x < LARGURA; x++)
      campo[y][x] = 0;

  pontuacao = 0;
  nivel = 0;
  linhasTotais = 0;
  intervaloQueda = 800;
  gameOver = false;

  proximaPeca = novaPecaAleatoria();
  spawnPeca();

  ultimaQueda = millis();
  musicaAtiva = true;

  Serial.println(F("=== NOVO JOGO ==="));
  Serial.println(F("Pontuacao: 0 | Nivel: 0"));
}

// ---------------------------------------------------------------------------
// ANIMAÇÕES DE TELA (início e game over)
// ---------------------------------------------------------------------------
void animacaoInicio() {
  limparMatriz();
  // efeito de "varredura" simples para indicar que está pronto pra começar
  for (int y = 0; y < ALTURA; y++) {
    for (int x = 0; x < LARGURA; x++) {
      setPixel(x, y, true);
    }
    delay(25);
  }
  delay(200);
  for (int y = ALTURA - 1; y >= 0; y--) {
    for (int x = 0; x < LARGURA; x++) {
      setPixel(x, y, false);
    }
    delay(25);
  }
  Serial.println(F("Pressione PAUSE/START para comecar!"));
}

void animacaoGameOver() {
  somGameOver();
  // preenche de baixo para cima, como no Tetris clássico
  for (int y = ALTURA - 1; y >= 0; y--) {
    for (int x = 0; x < LARGURA; x++) {
      setPixel(x, y, true);
    }
    delay(50);
  }
  delay(500);
  // pisca tudo
  for (int i = 0; i < 4; i++) {
    limparMatriz();
    delay(150);
    for (int y = 0; y < ALTURA; y++)
      for (int x = 0; x < LARGURA; x++)
        setPixel(x, y, true);
    delay(150);
  }
  limparMatriz();

  Serial.println(F("=== GAME OVER ==="));
  Serial.print(F("Pontuacao final: "));
  Serial.println(pontuacao);
  delay(2000);
}

// ---------------------------------------------------------------------------
// SETUP
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  // configura os dois módulos MAX7219
  lc.shutdown(TOPO_ADDR, false);
  lc.shutdown(FUNDO_ADDR, false);
  lc.setIntensity(TOPO_ADDR, 8);
  lc.setIntensity(FUNDO_ADDR, 8);
  lc.clearDisplay(TOPO_ADDR);
  lc.clearDisplay(FUNDO_ADDR);

  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_ROTATE, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_PAUSE, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);

  randomSeed(analogRead(A0));

  Serial.println(F("=== TETRIS Arduino Nano ==="));
  animacaoInicio();

  jogoAtivo = false;
  jogoPausado = true;
}

// ---------------------------------------------------------------------------
// LOOP PRINCIPAL
// ---------------------------------------------------------------------------
void loop() {

  // --- Tela de início: espera o jogador apertar PAUSE/START ---
  if (!jogoAtivo) {
    if (botaoPressionado(btnPause)) {
      resetarJogo();
      jogoAtivo = true;
      jogoPausado = false;
    }
    return;
  }

  // --- Game over: anima e reinicia ---
  if (gameOver) {
    desenharCampo();
    animacaoGameOver();
    jogoAtivo = false;
    return;
  }

  // --- Pausa ---
  if (botaoPressionado(btnPause)) {
    jogoPausado = !jogoPausado;
    if (jogoPausado) {
      musicaAtiva = false;
      noTone(BUZZER_PIN);
      Serial.println(F("--- PAUSADO ---"));
    } else {
      musicaAtiva = true;
      ultimaQueda = millis(); // evita queda instantânea ao despausar
      Serial.println(F("--- CONTINUANDO ---"));
    }
  }

  if (jogoPausado) {
    // pisca lentamente o campo para indicar pausa, mas mantém o desenho atual
    static unsigned long ultimoPisca = 0;
    static bool visivel = true;
    if (millis() - ultimoPisca > 500) {
      ultimoPisca = millis();
      visivel = !visivel;
      if (visivel) desenharCampo();
      else limparMatriz();
    }
    return;
  }

  // --- Música toca em paralelo, sempre que o jogo está ativo e não pausado ---
  atualizarMusica();

  // --- Leitura dos botões de movimento ---
  if (botaoPressionado(btnLeft) || botaoRepetindo(btnLeft)) {
    if (!colisao(pecaAtual, rotacaoAtual, pecaX - 1, pecaY)) {
      pecaX--;
    }
  }

  if (botaoPressionado(btnRight) || botaoRepetindo(btnRight)) {
    if (!colisao(pecaAtual, rotacaoAtual, pecaX + 1, pecaY)) {
      pecaX++;
    }
  }

  if (botaoPressionado(btnRotate)) {
    int novaRot = (rotacaoAtual + 1) % 4;
    if (!colisao(pecaAtual, novaRot, pecaX, pecaY)) {
      rotacaoAtual = novaRot;
      somRotacao();
    } else if (!colisao(pecaAtual, novaRot, pecaX - 1, pecaY)) {
      // tenta um pequeno "kick" para os lados se girar colidir
      rotacaoAtual = novaRot;
      pecaX--;
      somRotacao();
    } else if (!colisao(pecaAtual, novaRot, pecaX + 1, pecaY)) {
      rotacaoAtual = novaRot;
      pecaX++;
      somRotacao();
    }
  }

  // --- Queda automática (gravidade) e descida rápida ---
  unsigned long agora = millis();
  unsigned long intervaloEfetivo = intervaloQueda;

  bool descidaRapida = (digitalRead(btnDown.pino) == LOW);
  if (descidaRapida) {
    intervaloEfetivo = 50; // queda bem mais rápida enquanto o botão é mantido
  }

  if (agora - ultimaQueda >= intervaloEfetivo) {
    ultimaQueda = agora;
    if (!colisao(pecaAtual, rotacaoAtual, pecaX, pecaY + 1)) {
      pecaY++;
    } else {
      travarPecaEAvaliar();
    }
  }

  // --- Redesenha a matriz a cada ciclo ---
  desenharCampo();
}
