/* source: http://nicecircuits.com/playing-with-analog-to-digital-converter-on-arduino-due/ */

/**** INCLUDES ****/
#include <TFT.h>
#include <SPI.h>
#include <FreeRTOS_ARM.h>

/**** DEFINES ****/
// LCD pins
#define cs    10
#define dc    9
#define rst   8

// pot pins
#define pot1 A8
#define pot2 A9
#define pot3 A10
#define linein A11

/**** CONSTANTS ****/
const int _RESOLUTION = 4096;

/**** OBJECTS****/
TFT TFTScreen = TFT(cs, dc, rst);

/**** VARIABLES ****/
int barsColorRed[3]   = {255,0,0};
int barsColorGreen[3] = {0,255,0};
int barsColorBlue[3]  = {0,0,255};

int adc_value1, adc_value2, adc_value3;
adc_channel_num_t tag1, tag2, tag3;

/**** CONCURRENCIAL VARIABLES ****/
int pot1_value = 0, pot2_value = 0, pot3_value = 0;

/**** THREADS ****/
static void ThreadPotReader(void *arg) {
  while(1) {
    while ((adc_get_status(ADC) & ADC_ISR_DRDY) != ADC_ISR_DRDY)
      {}; //Wait for end of conversion
    tag1 = adc_get_tag(ADC);
    adc_value1 = adc_get_latest_value(ADC);
    

    while ((adc_get_status(ADC) & ADC_ISR_DRDY) != ADC_ISR_DRDY)
      {}; //Wait for end of conversion
    tag2 = adc_get_tag(ADC);
    adc_value2 = adc_get_latest_value(ADC);
    

    while ((adc_get_status(ADC) & ADC_ISR_DRDY) != ADC_ISR_DRDY)
      {}; //Wait for end of conversion
    tag3 = adc_get_tag(ADC);
    adc_value3 = adc_get_latest_value(ADC);
    
/*
    adc_value1 = (adc_value1 > 0)?adc_value1:0;
    adc_value2 = (adc_value2 > 0)?adc_value2:0;
    adc_value3 = (adc_value3 > 0)?adc_value3:0;
*/
    switch(tag1) {
      case ADC_CHANNEL_10:
        pot1_value = adc_value1 - _RESOLUTION * 10;
        break;

      case ADC_CHANNEL_11:
        pot2_value = adc_value1 - _RESOLUTION * 11;
        break;

      case ADC_CHANNEL_12:
        pot3_value = adc_value1 - _RESOLUTION * 12;
        break;
    }

    switch(tag2) {
      case ADC_CHANNEL_10:
        pot1_value = adc_value2 - _RESOLUTION * 10;
        break;

      case ADC_CHANNEL_11:
        pot2_value = adc_value2 - _RESOLUTION * 11;
        break;

      case ADC_CHANNEL_12:
        pot3_value = adc_value2 - _RESOLUTION * 12;
        break;
    }

    switch(tag3) {
      case ADC_CHANNEL_10:
        pot1_value = adc_value3 - _RESOLUTION * 10;
        break;

      case ADC_CHANNEL_11:
        pot2_value = adc_value3 - _RESOLUTION * 11;
        break;

      case ADC_CHANNEL_12:
        pot3_value = adc_value3 - _RESOLUTION * 12;
        break;
    }
/*
    Serial.print(pot1_value);
    Serial.print(" ::: ");
    Serial.print(pot2_value);
    Serial.print(" ::: ");
    Serial.println(pot3_value);
*/
/*
    Serial.print(tag1);
    Serial.print(" ::: ");
    Serial.print(tag2);
    Serial.print(" ::: ");
    Serial.println(tag3);
*/
  vTaskDelay((10L * configTICK_RATE_HZ) / 1000L);
  }
}

static void ThreadPotWriter(void *arg) {
  while(1) {
    putBars(pot1_value, 0, 4095, 30, 82, 160, barsColorGreen);
    putBars(pot2_value, 0, 4095, 30, 98, 160, barsColorBlue);
    putBars(pot3_value, 0, 4095, 30, 114, 160, barsColorRed);
    vTaskDelay((50L * configTICK_RATE_HZ) / 1000L);
  }
}

/**** MAIN ****/
void setup()
{
  portBASE_TYPE s1, s2;
  
  pinMode(pot1,INPUT);
  pinMode(pot2,INPUT);
  pinMode(pot3,INPUT);

  Serial.begin(115200); // testes

  // Setup all registers
  pmc_enable_periph_clk(ID_ADC); // To use peripheral, we must enable clock distributon to it
  adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST); // initialize, set maximum possible speed
  
  adc_disable_interrupt(ADC, 0xFFFFFFFF);
  adc_set_resolution(ADC, ADC_12_BITS); // não consigo passar para 10, se o fizer, fica na mesma com resoltução de 12 bits
  
  adc_configure_power_save(ADC, 0, 0); // Disable sleep
  adc_configure_timing(ADC, 0, ADC_SETTLING_TIME_3, 1); // Set timings - standard values
  adc_set_bias_current(ADC, 1); // Bias current - maximum performance over current consumption
  adc_disable_ts(ADC); // disable temperature sensor
  
  enum adc_channel_num_t adc_channel_list[3] = {ADC_CHANNEL_10, ADC_CHANNEL_11, ADC_CHANNEL_12};
  adc_configure_sequence(ADC, adc_channel_list, 3); // prepara a lista de portas a serem lidas

  adc_enable_tag(ADC);
  
  adc_disable_channel_differential_input(ADC, ADC_CHANNEL_10); // A8 - 1º potencimetro
  adc_disable_channel_differential_input(ADC, ADC_CHANNEL_11); // A9 - 2º potencimetro
  adc_disable_channel_differential_input(ADC, ADC_CHANNEL_12); // A10 - 3º potencimetro
  //adc_enable_channel_differential_input(ADC, ADC_CHANNEL_13); // line-in
  
  adc_configure_trigger(ADC, ADC_TRIG_SW, 1); // triggering from software, freerunning mode
  
  adc_disable_all_channel(ADC);
  adc_enable_channel(ADC, ADC_CHANNEL_10); // A8 
  adc_enable_channel(ADC, ADC_CHANNEL_11); // A9
  adc_enable_channel(ADC, ADC_CHANNEL_12); // A10
  //adc_enable_channel(ADC, ADC_CHANNEL_13); // A11

  adc_start(ADC);
//  adc_start_sequencer(ADC); // só funciona sem isto - estranhamente

  initDisplay();

  // create tasks
  s1 = xTaskCreate(ThreadPotReader, "readPotValue", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
  s2 = xTaskCreate(ThreadPotWriter, "writePotValue", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  // check for creation errors
  if ((s1 != pdPASS) || (s2 != pdPASS)) {
    displayError();
    while(1);
  }
  // start scheduler
  vTaskStartScheduler();
  displayError();
  while(1);
}

void loop() 
{
  // do nothing
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
