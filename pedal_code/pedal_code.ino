/* source: http://nicecircuits.com/playing-with-analog-to-digital-converter-on-arduino-due/ */



/* fopr tests */
//PIO_Set(PIOB,PIO_PB27B_TIOB0);
//PIO_Clear(PIOB,PIO_PB27B_TIOB0);

/**** INCLUDES ****/
#include <TFT.h>
#include <SPI.h>
#include <FreeRTOS_ARM.h>

/**** DEFINES ****/
// LCD pins
#define cs    10
#define dc    9
#define rst   8

// pins
#define pot1 A8
#define pot2 A9
#define pot3 A10
#define linein_pin A0
#define lineout_pin DAC1
#define direct_line_pin 52

#define BUFFER_SIZE_IN 40000
#define OVERDRIVE_LIMIT 200

/**** RTOS CONFIGURATIONS ****/
// nada

/**** CONSTANTS ****/
const int _RESOLUTION = 4096;
const double echo_matrix[5][5] = {{1, 0,     0,     0,     0}, 
                                  {1, 1.0/2, 0,     0,     0}, 
                                  {1, 2.0/5, 2.0/5, 0,     0}, 
                                  {1, 3.0/7, 2.0/7, 2.0/7, 0}, 
                                  {1, 3.0/9, 2.0/9, 2.0/9, 2.0/9}};

/**** OBJECTS****/
TFT TFTScreen = TFT(cs, dc, rst);

/**** VARIABLES ****/
int barsColorRed[3] = {255,0,0};
int barsColorGreen[3] = {0,255,0};
int barsColorBlue[3] = {0,0,255};
int barsColorYellow[3] = {255,255,0};
int barsColorBlack[3] = {0,0,0};

// circular buffers
unsigned point = 0;
unsigned short num_samples = 0;

/**** VARIABLES ****/
int pot1_value = 0, pot2_value = 0, pot3_value = 0;
int n_echo = 0, spacing = 0, direct_line, old_direct_line, overdrive = 1;
int mean = 0, count = 0;

short buffer_in[BUFFER_SIZE_IN] = {0};
short vrms[500] = {0};


/**** HEADERS ****/
void putBars(int value, int min, int max, int beginX, int beginY, int end, int color[]);
void initDisplay();
void setDisplay();
void displayError();
short echo_test();
void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency);
void makeAmplitude(byte X, byte Y, byte len);
void makeAmplitude2(byte X, byte Y, byte len);
void putConnectionType();

int teste = 0;
unsigned long time1, time2;

/**** INTERRUPTS ****/
void TC3_Handler()
{
  TC_GetStatus(TC1, 0);
  int atraso;
  short value = 0;

  buffer_in[point] = adc_get_channel_value(ADC, ADC_CHANNEL_7);

  if (direct_line == LOW) {
    //overdrive
      
        buffer_in[point] = buffer_in[point] * overdrive;
        if (buffer_in[point] > OVERDRIVE_LIMIT) {
          buffer_in[point] = OVERDRIVE_LIMIT;
        }
      

    //echo
    for (int n = 0; n <= n_echo; n++) {
      atraso = point - n*spacing;
  
      if (atraso < 0) {
        atraso = BUFFER_SIZE_IN + atraso;
      }
  
      value = value + (short)(buffer_in[atraso] * echo_matrix[n_echo][n]); 

      PIO_Clear(PIOD,PIO_PD1);
    }
  } else {
    value = (buffer_in[point]);
  }

  analogWrite(lineout_pin, value);  

  if (count < 500) {
    vrms[count] = buffer_in[point];
    count++;
  }

  point = (++point)%BUFFER_SIZE_IN;
}

/**** THREADS ****/
static void ThreadPotReader(void *arg) {
  while (1) {
    
    pot1_value = adc_get_channel_value(ADC, ADC_CHANNEL_10);
    overdrive = map(pot1_value, 0, 4095, 1, 30);
    vTaskDelay(1L);
    
    pot2_value = adc_get_channel_value(ADC, ADC_CHANNEL_11);
    n_echo = map(pot2_value, 0, 4095, 0, 5);
    n_echo = (n_echo > 4) ? 4 : n_echo;
    vTaskDelay(1L);

    pot3_value = adc_get_channel_value(ADC, ADC_CHANNEL_12);
    spacing = map(pot3_value, 0, 4095, 0, 40000/n_echo);
    vTaskDelay(1L);
    
    direct_line = digitalRead(direct_line_pin);
    vTaskDelay(1L);
  }
}

static void ThreadPotWriter(void *arg) {
  int mean_write;
  char num_to_print[5] = {0};
  int x = 0, y = 0, len = 5;
  
  while (1) {
    putBars(pot1_value, 0, 4095, 30, 82, 160, barsColorGreen);
    vTaskDelay(1L);
    
    //putBars(pot2_value, 0, 4095, 30, 98, 160, barsColorBlue);
    putBars(n_echo, 0, 4, 30, 98, 52, barsColorBlue);
    vTaskDelay(1L);

    putBars(pot3_value, 0, 4095, 30, 114, 160, barsColorRed);
    vTaskDelay(1L);
    
    putConnectionType();
    vTaskDelay(1L);
    
    long sum = 0;
    for (int i = 0; i < 500; i++) {
      sum += vrms[i] * vrms[i];
    }  
    
    sum = (long)(sum/500.0);
    sum = sqrt(sum);
    
    y = map(sum, 0, 150, 0, 40);
    if (y > 40) {
      y = 40;
    }

    noInterrupts();
    count = 0;
    interrupts();
    
    makeAmplitude(x, y, len);
    x = (x+len)%160;
    vTaskDelay(1L);
  }
}

/**** MAIN ****/
void setup()
{
  portBASE_TYPE s1, s2;
  
  pinMode(pot1,INPUT);
  pinMode(pot2,INPUT);
  pinMode(pot3,INPUT);
  pinMode(linein_pin,INPUT);
  pinMode(13, OUTPUT);
  pinMode(direct_line_pin,INPUT);

  //Serial.begin(115200); // testes

  // Setup all registers
  pmc_enable_periph_clk(ID_ADC); // To use peripheral, we must enable clock distributon to it
  adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST); // initialize, set maximum possible speed
  
  adc_disable_interrupt(ADC, 0xFFFFFFFF);
  adc_set_resolution(ADC, ADC_12_BITS); // não consigo passar para 10, se o fizer, fica na mesma com resoltução de 12 bits
  
  adc_configure_power_save(ADC, 0, 0); // Disable sleep
  adc_configure_timing(ADC, 0, ADC_SETTLING_TIME_3, 1); // Set timings - standard values
  adc_set_bias_current(ADC, 1); // Bias current - maximum performance over current consumption
  adc_disable_ts(ADC); // disable temperature sensor
  
  enum adc_channel_num_t adc_channel_list[4] = {ADC_CHANNEL_10, ADC_CHANNEL_11, ADC_CHANNEL_12, ADC_CHANNEL_7};
  adc_configure_sequence(ADC, adc_channel_list, 4); // prepara a lista de portas a serem lidas

  adc_enable_tag(ADC);
  
  adc_disable_channel_differential_input(ADC, ADC_CHANNEL_10); // A8 - 1º potencimetro
  adc_disable_channel_differential_input(ADC, ADC_CHANNEL_11); // A9 - 2º potencimetro
  adc_disable_channel_differential_input(ADC, ADC_CHANNEL_12); // A10 - 3º potencimetro
  adc_enable_channel_differential_input(ADC, ADC_CHANNEL_7); // line-in
  
  adc_configure_trigger(ADC, ADC_TRIG_SW, 1); // triggering from software, freerunning mode
  
  adc_disable_all_channel(ADC);
  adc_enable_channel(ADC, ADC_CHANNEL_10); // A8 
  adc_enable_channel(ADC, ADC_CHANNEL_11); // A9
  adc_enable_channel(ADC, ADC_CHANNEL_12); // A10
  adc_enable_channel(ADC, ADC_CHANNEL_7); // A11

  adc_start(ADC);

  analogWriteResolution(12); // resolução para a escrita analógica

  initDisplay();

  // create tasks
  s1 = xTaskCreate(ThreadPotReader, "readPotValue", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
  s2 = xTaskCreate(ThreadPotWriter, "writePotValue", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  //TIMERS
  startTimer(TC1, 0, TC3_IRQn, 20000); //timer a dar frequência de amostragem de 20kHz

  // check for creation errors
  //if ((s1 != pdPASS) || (s2 != pdPASS)) {
  //  displayError();
  //  while(1);
  //}
 
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
  n_bars = map(value, min, max, 0, max_bars + 1); // número de barras a usar
  n_bars = (n_bars > max_bars)?max_bars:n_bars;

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

  TFTScreen.stroke(0,0,0);
  direct_line = digitalRead(direct_line_pin);
  old_direct_line = direct_line;
  TFTScreen.setTextSize(3);

  if (direct_line == LOW) {
    TFTScreen.text("on", 64, 56);
  } else {
    TFTScreen.text("off", 55, 56);
  }
  
}

void setDisplay() {
  TFTScreen.background(255,255,255); //  fundo branco
  TFTScreen.setTextSize(2);
  TFTScreen.stroke(0, 0, 0);
  TFTScreen.text("P1", 0, 82);
  TFTScreen.text("P2", 0, 98);
  TFTScreen.text("P3", 0, 114);
  //TFTScreen.text("LI", 0, 66);
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

void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency) {
        pmc_set_writeprotect(false);
        pmc_enable_periph_clk((uint32_t)irq);
        
        TC_Configure(tc, channel, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4);
        
        uint32_t rc = VARIANT_MCK/128/frequency; //128 because we selected TIMER_CLOCK4 above
        TC_SetRA(tc, channel, rc/2); //50% high, 50% low
        TC_SetRC(tc, channel, rc);
        TC_Start(tc, channel);
        
        tc->TC_CHANNEL[channel].TC_IER=TC_IER_CPCS;
        tc->TC_CHANNEL[channel].TC_IDR=~TC_IER_CPCS;
        
        NVIC_EnableIRQ(irq);
}

void makeAmplitude(byte X, byte Y, byte len) {
    TFTScreen.noStroke();
    TFTScreen.fill(255,255,255);
    TFTScreen.rect(X, 10, len, 42);
    TFTScreen.fill(0,0,0);
    TFTScreen.rect(X, 50 - Y, len, Y);
}


void makeAmplitude2(byte X, byte Y, byte len) {
    TFTScreen.noStroke();
    TFTScreen.fill(255,255,255);
    
    if ((X + len) >= 160) {
      TFTScreen.rect(0, 10, len, 42);
    } else {
      TFTScreen.rect(X + len, 10, len, 42);
    }
    
    TFTScreen.fill(0,0,0);
    TFTScreen.rect(X, 50 - Y, len, 1);
}

void putConnectionType() {
  if (direct_line != old_direct_line) {
    old_direct_line = direct_line;
    
    TFTScreen.noStroke(); // retira a linha
    TFTScreen.fill(255,255,255);
    TFTScreen.rect(55, 56, 60, 22); // limpa barras anteriores
    TFTScreen.stroke(0,0,0);
    TFTScreen.setTextSize(3);

    if (old_direct_line == LOW) {
      TFTScreen.text("on", 64, 56);
    } else {
      TFTScreen.text("off", 55, 56);
    }
  }
}
