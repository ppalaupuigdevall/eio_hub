
//Initialization
//Libraries Definition

#include <Wire.h>                 //I2C Library
#include <Adafruit_ADS1015.h>     //ADC Lybrary
#include <IRremote.h>             //Remote IR Library
//Definition of Sensors Addresses
#define light  0x23         //Light Sensor Address 0x23
#define humidity 0x27       //Humidity Sensor Address 0x27
Adafruit_ADS1115 ads(0x48); //Temperature Sensor Address 0x48

IRsend irsend;
//Variables for the Temperature Calculations
float Voltage = 0.0;
int Beta = 4540;
float To = 298.15;
float R1 = 1500000; // Rfixed for generating a Voltage Divisor
float Ro=100000;
float R;
float TempFin;

//These 14 Bytes are: initSeq (2B) + temp (4B) + press (4B) + humidity (4B) 
#define SEQ_LEN_NO_CRC 14 // In Bytes

//definitions

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

//Measurements from the sensors
//Temperature
float readTemp() {
  int16_t adc0;  // we read from the ADC, we have a sixteen bit integer as a result

  adc0 = ads.readADC_SingleEnded(3);
  Voltage = (adc0 * 0.1875) / 1000; //Each value means 0.1875mV
  R=(R1*5/Voltage)-R1;              //Based on Tension Divisor
  float Temp=-273+Beta*To/(Beta + To*log(R/Ro));// Formula from Thermistor behaviour 
  Serial.print("\Temperature: ");
  Serial.print(Temp);
  Serial.println("ºC");
  return Temp;
  delay(100);// before was 1000
}

//Humidity
float readHumidity() {
  Wire.beginTransmission(humidity);
  //Wire.write(sht21humid);// check lecture code
  Wire.endTransmission();
  delay(100);
  Wire.requestFrom(humidity, 3);
 
  byte msb = Wire.read();
  byte lsb = Wire.read();
  int h = (msb << 8) + lsb;
  int crc = Wire.read();
 
  float hum =  h *100.0/ (16384.0-2.0); //Got from the datasheet
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println("%");
  return hum;}

//Luminiscense
float readLight() {
  Wire.beginTransmission(light);
  Wire.write(0x11);// check lecture code
  Wire.endTransmission();
  delay(100);
  Wire.requestFrom(light, 3);
 
  byte msb = Wire.read();
  byte lsb = Wire.read();
  int l = (msb << 8) + lsb;
  int crc = Wire.read();
 
  float lum = l / 1.2;  //Got from datasheet
  Serial.print("Light: ");
  Serial.print(lum);
  Serial.println("lux");
  Serial.print("\n");
  return lum;}
  
  
void setup() 
{
  Serial.begin(9600);
  ads.begin();
  while(!Serial); 
  Serial.println("---- NO CONNECTION :(((((  ----");
  Wire.begin();
}

//CRC Calculation
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
  //Serial.print(len, DEC);
  for(byte i = 0; i <SEQ_LEN_NO_CRC; i++){
    count = count + compute_CRC(arr[i]);
  }
  return count;
}


void loop() {

  // Humidity reading
  float hum= readHumidity();

  // Temperature reading
  float Temp = readTemp();

  // Light reading
  float lum = readLight();

//Frame in order to send to the HUB group

  //This is the sequence over which we'll compute the CRC
  byte sequence[SEQ_LEN_NO_CRC];
  byte initSeq0 = 0xb0;
  byte initSeq1 = 0xb0;
  sequence[0] = initSeq0;
  sequence[1] = initSeq1;

  //TEMPERATURE burst preparation
  byte temp_init_seq = 0xbf;
  float temp_added_20 = Temp + 20.0;
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

  //LIGTH burst preparation
  byte light_init_seq = 0xbe;
  byte es_par;
  unsigned int part_entera = (unsigned int) (lum/2);
  if((part_entera%2) == 0){
    es_par = 0x00;
  }else{
    es_par = 0xff;
  }
  uint2bytes light_2_bytes;
  light_2_bytes.myInt = part_entera;
  burst light_bin;
  light_bin.myByte[3] = light_init_seq;
  light_bin.myByte[2] = es_par;
  light_bin.myByte[1] = light_2_bytes.byte_array[1];
  light_bin.myByte[0] = light_2_bytes.byte_array[0];
  
  for(byte i=0; i<4; i++){
    sequence[i+2 +4] = light_bin.myByte[i];
  }
  delay(100);

  //HUMIDITY burst preparation
  byte humidity_init_seq = 0xb5;
  int humidity_int = (int) hum;
  byte humidity_byte = (byte) humidity_int;
  float humidity_dec_part = hum - humidity_int;
  int humidity_dec_part_int = (int) (humidity_dec_part * 1000);
  int2bytes converter_humidity;
  converter_humidity.myInt = humidity_dec_part_int;
  burst humidity_burst;
  humidity_burst.myByte[3] = humidity_init_seq;
  humidity_burst.myByte[2] = humidity_byte;
  humidity_burst.myByte[1] = converter_humidity.byte_array[1];
  humidity_burst.myByte[0] = converter_humidity.byte_array[0];
  
  for(byte i=0; i<4; i++){
    sequence[i+2 +4 +4 ] = humidity_burst.myByte[i];
  }
  
  delay(100);

  //CRC computation
  byte crc = compute_array_CRC(sequence);
  //We create the first burst (32 bits = 4B : initseq0(1B) + initseq1(1B) + crc(1B) + padding(1B)
  burst first;
  first.myByte[3] = initSeq0;
  first.myByte[2] = initSeq1;
  first.myByte[0] = crc;
  first.myByte[1] = 0xee; //Padding

  //Send all the sequences
  Serial.println(crc, HEX);
  //Send all the sequences
  Serial.println(first.myLong, HEX);
  irsend.sendNEC(first.myLong, 32);
  delay(300);
  Serial.println(temp_burst.myLong, HEX);
  irsend.sendNEC(temp_burst.myLong, 32);
  delay(300);
  Serial.println(light_bin.myLong, HEX);
  irsend.sendNEC(light_bin.myLong, 32);
  delay(300);
  Serial.println(humidity_burst.myLong, HEX);
  irsend.sendNEC(humidity_burst.myLong, 32);
  delay(10000);
}
