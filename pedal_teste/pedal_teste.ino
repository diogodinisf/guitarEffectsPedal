#include <TFT.h>
#include <SPI.h>

/**** DEFINES ****/
// LCD pins
#define cs    10
#define dc    9
#define rst   8

// pot pins
#define pot1  A8
#define pot2  A9
#define pot3  A10

/**** OBJECTS****/
TFT TFTScreen = TFT(cs, dc, rst);

/**** VARIABLES ****/
int barsColorRed[3]   = {255,0,0};
int barsColorGreen[3] = {0,255,0};
int barsColorBlue[3]  = {0,0,255};

/**** CONCURRENCIAL VARIABLES ****/
int pot1_value = 0, pot2_value = 0, pot3_value = 0;

/**** HEADERS ****/
void putBars(int value, int min, int max, int beginX, int beginY, int end, int color[]);
void initDisplay();
void setDisplay();
void displayError();

/**** MAIN ****/
void setup() {
  initDisplay();
}

void loop() {
  pot1_value = analogRead(pot1);
  pot2_value = analogRead(pot2);
  pot3_value = analogRead(pot3);

  putBars(pot1_value, 0, 1023, 30, 82, 160, barsColorGreen);
  putBars(pot2_value, 0, 1023, 30, 98, 160, barsColorBlue);
  putBars(pot3_value, 0, 1023, 30, 114, 160, barsColorRed);
}

/**** FUNCTIONS ****/
void putBars(int value, int min, int max, int beginX, int beginY, int end, int color[]) {
  int max_bars;
  int n_bars;

  max_bars = (end - beginX) / 6 + 1; // arredondamento por truncatura + não ter barra
  n_bars = map(value, min, max, 0, max_bars); // número de barras a usar

  TFTScreen.noStroke(); // retira a linha
  TFTScreen.fill(255,255,255);
  TFTScreen.rect(beginX, beginY, end - beginX, 14); // limpa barras anteriores
  
  TFTScreen.fill(color[0], color[1], color[2]); // cor preenchimento

  for (int i = 0; i < n_bars; i++) {
    TFTScreen.rect(beginX + 6*i, beginY, 4, 14);
  }
  
  TFTScreen.noFill(); // retira o preenchimento
}

void initDisplay() {
  TFTScreen.begin();
  setDisplay();
}

void setDisplay() {
  TFTScreen.background(255,255,255); //  fundo branco
  TFTScreen.setTextSize(2);
  TFTScreen.stroke(0, 0, 0);
  TFTScreen.text("P1", 0, 82);
  TFTScreen.text("P2", 0, 98);
  TFTScreen.text("P3", 0, 114);
}

void displayError() {
  TFTScreen.background(255,255,255); //  fundo branco

  TFTScreen.stroke(255, 0, 0);
  TFTScreen.setTextSize(4);
  TFTScreen.text("ERROR", 20, 50);
  TFTScreen.stroke(0, 0, 0);
  TFTScreen.setTextSize(1);
  TFTScreen.text("RESET YOUR PEDAL", 20, 80);
}

