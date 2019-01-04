#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>      //pressure
#include <Adafruit_ADS1015.h>     //temperature
#include <MAX44009.h>
#include <IRremote.h>

#define SEQ_LEN_NO_CRC 14

IRsend irsend;

Adafruit_BMP280 bme; //pressure
Adafruit_ADS1115 ads(0x48); //temperature
MAX44009 senslight; //light

float pressure;

float Voltage = 0.0;
int thermistor_25 = 10000;
float refCurrent = 0.0001;
int16_t adc0;
float resistance;
float ln;
float kelvin;
float temp;
float temp_to_tx;

float light;

typedef union{
  byte myByte[4];
  unsigned long myLong;
}burst;

typedef union {
  int floatingPoint;
  byte binary[4];
}binaryFloat;

typedef union {
  int myInt;
  byte byte_array[2];
}int2bytes;

typedef union{
  unsigned int myInt;
  byte byte_array[2];
}uint2bytes;

void setup() {
  Serial.begin(9600);

  if (!bme.begin()) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  ads.begin();

  Wire.begin();
}

byte compute_CRC(byte b){
  byte count = 0;
  while(b != 0){
    if(b%2 ==1){
      count++;
    }
    b /= 2;
  }
  return count;
}

byte compute_array_CRC(byte *arr){
  byte count = 0;
  byte len = sizeof(arr);

  for(byte i = 0; i < SEQ_LEN_NO_CRC; i++){
    count = count + compute_CRC(arr[i]);
  }
  return count;
}

float pressureReading(){
  pressure = bme.readPressure();
  return pressure;
}

float temperatureReading(){
  adc0 = ads.readADC_SingleEnded(0); // Read ADC value from ADS1115
  Voltage = adc0 * (5.0 / 65535); // Replace 5.0 with whatever the actual Vcc of your Arduino is
  resistance = (Voltage / refCurrent); // Using Ohm's Law to calculate resistance of thermistor
  ln = log(resistance / thermistor_25); // Log of the ratio of thermistor resistance and resistance at 25 deg. C
  kelvin = 1 / (0.0033540170 + (0.00025617244 * ln) + (0.0000021400943 * ln * ln) + (-0.000000072405219 * ln * ln * ln)); // Using the Steinhart-Hart Thermistor Equation to calculate temp in K
  temp = kelvin - 273.15; // Converting Kelvin to Celcuis
  return temp;
}

float lightReading(){
  light = senslight.get_lux();
  return light;
}

void loop() {

  // Pressure reading
  pressure = pressureReading();

  // Temperature reading
  temp_to_tx = temperatureReading();

  // Light reading
  light = lightReading();

  Serial.println("Temperature:");
  Serial.println(temp_to_tx);

  Serial.println("Light:");
  Serial.println(light);

  Serial.println("Pressure:");
  Serial.println(pressure);
  Serial.println();

  //This is the sequence over which we'll compute the CRC
  byte sequence[SEQ_LEN_NO_CRC];
  //The initSeq for this sender node is 0xaa, 0xaa
  byte initSeq0 = 0xa0;
  byte initSeq1 = 0xa0;
  sequence[0] = initSeq0;
  sequence[1] = initSeq1;

  //TEMPERATURE burst preparation
  byte temp_init_seq = 0xaf;
  float temp_added_20 = temp_to_tx + 20.0;
  int temp_int = (int) temp_added_20;
  byte temp_byte = (byte) temp_int;
  float dec_part = temp_added_20 - temp_int;
  int dec_part_int = (int) (dec_part * 1000);
  int2bytes converter;
  converter.myInt = dec_part_int;
  burst temp_burst;
  temp_burst.myByte[3] = temp_init_seq;
  temp_burst.myByte[2] = temp_byte;
  temp_burst.myByte[1] = converter.byte_array[1];
  temp_burst.myByte[0] = converter.byte_array[0];

  for(byte i=0; i<4; i++){
    sequence[i+2] = temp_burst.myByte[i];
  }

  delay(100);

  //PRESSURE burst preparation
  byte pressure_init_seq = 0xaa;
  byte es_par;
  unsigned int part_entera = (unsigned int) (pressure/2);
  if((part_entera%2) == 0){
    //Si estem aqui (PARELL) el receptor no haura de sumar res, nomes multiplicar per 2
    es_par = 0x00;
  }else{
    //Si estem aqui (IMPARELL) el receptor haura de sumar 1 al resultat
    es_par = 0xff;
  }
  uint2bytes pressure_2_bytes;
  pressure_2_bytes.myInt = part_entera;
  burst press_bin;
  press_bin.myByte[3] = pressure_init_seq;
  press_bin.myByte[2] = es_par;
  press_bin.myByte[1] = pressure_2_bytes.byte_array[1];
  press_bin.myByte[0] = pressure_2_bytes.byte_array[0];
  
  for(byte i=0; i<4; i++){
    sequence[i+2 +4] = press_bin.myByte[i];
  }
  delay(100);

  //LIGHT burst preparation
  byte light_init_seq = 0xa5;
  int light_int = (int) light;
  byte light_byte = (byte) light_int;
  float light_dec_part = light - light_int;
  int light_dec_part_int = (int) (light_dec_part * 1000);
  int2bytes converter_light;
  converter_light.myInt = light_dec_part_int;
  burst light_burst;
  light_burst.myByte[3] = light_init_seq;
  light_burst.myByte[2] = light_byte;
  light_burst.myByte[1] = converter_light.byte_array[1];
  light_burst.myByte[0] = converter_light.byte_array[0];
  
  for(byte i=0; i<4; i++){
    sequence[i+2 +4 +4 ] = light_burst.myByte[i];
  }
  
  delay(100);

  //CRC computation
  byte crc = compute_array_CRC(sequence);
  //We create the first burst (32 bits = 4B : initseq0(1B) + initseq1(1B) + crc(1B) + padding(1B)
  burst first;
  first.myByte[3] = initSeq0;
  first.myByte[2] = initSeq1;
  first.myByte[1] = crc;
  first.myByte[0] = 0xee; //Padding

  //Send all the sequences
  Serial.println(crc, HEX);
  //Send all the sequences
  Serial.println(first.myLong, HEX);
  irsend.sendNEC(first.myLong, 32);
  delay(300);
  Serial.println(temp_burst.myLong, HEX);
  irsend.sendNEC(temp_burst.myLong, 32);
  delay(300);
  Serial.println(press_bin.myLong, HEX);
  irsend.sendNEC(press_bin.myLong, 32);
  delay(300);
  Serial.println(light_burst.myLong, HEX);
  irsend.sendNEC(light_burst.myLong, 32);
  delay(500);
}
