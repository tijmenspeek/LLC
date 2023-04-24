#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

unsigned int T1;
signed int T2;
signed int T3;
int segmentDot = 0b00000001; 

int segmentArray[10] = {0b01111110, // 0
                        0b00001100, // 1
                        0b10110110, // 2
                        0b10011110, // 3
                        0b11001100, // 4
                        0b11011010, // 5
                        0b11111010, // 6
                        0b00001110, // 7
                        0b11111110, // 8
                        0b11011110};// 9    
   
                      
void setModus();
void getCalibrationData();
float readSensor();
void updateShiftRegister(int output);
uint8_t transfer(uint8_t input);
void init();


int main(){
    _delay_ms(2000); //give ATtiny time to boot
    init(); //initialise 
    getCalibrationData();
    setModus();

    while(1){
      float temp_raw = readSensor();
      int tempTens =(int)( temp_raw / 10);
      int tempSingle = (int)temp_raw % 10;
      int tempDecimal = ((int)(temp_raw * 10))% 10;
      int tempTensDecimal = ((int)(temp_raw * 100))% 10;


        updateShiftRegister(segmentArray[tempTens]);
        _delay_ms(500);
        updateShiftRegister(segmentArray[tempSingle] + segmentDot);
        _delay_ms(500);
        updateShiftRegister(segmentArray[tempDecimal]);
        _delay_ms(500);
        updateShiftRegister(segmentArray[tempTensDecimal]);
        _delay_ms(1500);
    }
  return 0;
}


void init(){
  PORTB = 0; // set all pins to low

  DDRB |= 1 << DDB1; //set to output
  DDRB |= 1 << DDB2; //set to output
  DDRB |= 1 << DDB3; // pin 2 output
  DDRB |= 1 << DDB4; // pin 3 output

  USICR |= (1<<USIWM0) | (1<<USICS1); //set pins for SPI
}


uint8_t transfer(uint8_t input){ //transfer funcion puts data in de data register
  USIDR = input;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    for (int i = 0; i < 16; i++){
      USICR |= (1 << USITC);
    }
  }
  return USIDR;
}


void updateShiftRegister(int output){
   PORTB &= ~(1<<PB3); //set latchpin low
   transfer(output);
   PORTB |= (1<<PB3);  //set latchpin high
}


float readSensor(){
  //check if sensor works by requesting chip select pin for ID
  /*
  PORTB &= ~(1<<PB4);
  transfer(0xD0);
  uint8_t data = transfer(0x00);
  PORTB |= (1<<PB4);

  if(data == 0x58){
    updateShiftRegister(segmentArray[1]);
    _delay_ms(10000);
  }else{
    updateShiftRegister(segmentArray[0]);
  }
  */
  

 PORTB &= ~(1<<PB4); //set latchpin low
  transfer(0xFA); //request temp data from sensor
  long int adc_T;
  uint32_t TempA = transfer(0x00);
  uint32_t TempB = transfer(0x00);
  uint32_t TempC = transfer(0x00);
  PORTB |= (1<<PB4); //set latchpin high

  adc_T = (TempA<<12)|(TempB<<4)|(TempC>>4); //reconstruct requested data by bitshifting the order
  long  var1 = (((( adc_T  >> 3) - ((long)T1 << 1))) * ((long)T2)) >> 11;
  long  var2 = ((((( adc_T  >> 4) - ((long)T1)) * ((adc_T  >> 4) - ((long)T1))) >> 12) * ((long)T3)) >> 14;
  float temp = (var1 + var2) / 5120.0;

  return temp;
}


void getCalibrationData(){
  PORTB &= ~(1<<PB4);
  transfer(0x88); //request calibration data from sensor
  uint8_t t1_1 = transfer(0x00);
  uint8_t t1_2 = transfer(0x00);
  T1 = (t1_2 << 8)|(t1_1); //reconstruct T1 by bitshifting 
  uint8_t t2_1 = transfer(0x00);
  uint8_t t2_2 = transfer(0x00);
  T2 = (t2_2 << 8)|(t2_1); //reconstruct T2 by bitshifting 
  uint8_t t3_1 = transfer(0x00);
  uint8_t t3_2 = transfer(0x00);
  T3 = (t3_2 << 8)|(t3_1); //reconstruct T3 by bitshifting 
  PORTB |= (1 << PB4);
}


void setModus(){ //set sensor to right mode
  PORTB &= ~(1 << PB4);
  transfer(0x74);
  transfer(0b01000011);
  PORTB |= (1 << PB4);
}

