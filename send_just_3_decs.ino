#include <IRremote.h>

//These 14 Bytes are: initSeq (2B) + temp (4B) + press (4B) + humidity (4B) 
#define SEQ_LEN_NO_CRC 14 // In Bytes

IRsend irsend;

void setup()
{
  Serial.begin(9600);
}

/*Let us suppose that what we receive from the other groups is a float (4 Bytes) for each measurement.*/
float readings_from_sensors[3] = {10.41, 109999.2, 89.27};

/*A burst is a 4 Bytes sequence*/
typedef union{
  byte myByte[4];
  unsigned long myLong;
}burst;

typedef union {
 float floatingPoint;
 byte binary[4];
}binaryFloat;

typedef union{
  unsigned int myInt;
  byte byte_array[2];
}uint2bytes;

typedef union {
  int myInt;
  byte byte_array[2];
}int2bytes;

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
  Serial.println(len, DEC);
  for(byte i = 0; i <SEQ_LEN_NO_CRC; i++){
    count = count + compute_CRC(arr[i]);
  }
  return count;
}

void loop() {

  /*
    The code from the other groups should be hopefully something like:
    float readings_from_sensors[3];
    readings_from_sensors[0] = readTemp();
    readings_from_sensors[1] = readPress();
    readings_from_sensors[2] = readHumi();
  */
  
  //This is the sequence over which we'll compute the CRC
  byte sequence[SEQ_LEN_NO_CRC];
  byte initseq0 = 0xa0;
  byte initseq1 = 0xa0;
  sequence[0] = initseq0;
  sequence[1] = initseq1;
  
//TEMPERATURA
  /*The Packet structure for the temperature will be: For the node A: [AF, Integer_Part, Decimal_part, Decimal_part]
                                                      For the node B: [BF, Integer_Part, Decimal_part, Decimal_part]
  */
  float tempera = readings_from_sensors[0];

  //TEMPERATURE: fornode A temp_init_seq is 0xaf, for node B is 0xbf
  byte temp_init_seq = 0xaf;
  //Doing this we send just positive numbers but in reception we know that we have to substract 20 again
  float tempera_added_20 = tempera + 20.0;
  int tempera_int = (int) tempera_added_20;
  //This byte below will be send as integer_part
  byte tempera_byte = (byte) tempera_int;
  float decimal_part = tempera_added_20 - tempera_int;
  //This int below will be sent as decimal part
  int decimal_part_int = (int) (decimal_part * 1000); //We get 3 decimals from the number
  int2bytes converter;
  converter.myInt = decimal_part_int;
  burst temp_burst;
  temp_burst.myByte[3] = temp_init_seq;
  temp_burst.myByte[2] = tempera_byte;
  temp_burst.myByte[1] = converter.byte_array[0];
  temp_burst.myByte[0] = converter.byte_array[1];
  for(byte i=0; i<4; i++){
    sequence[i+2] = temp_burst.myByte[i];
  }
  delay(100);

//PRESSIO 
/*The Packet structure for the pressure will be:      For the node A: [A0, ODD_EVEN, Integer_part, Integer_part]
                                                      For the node B: [B0, ODD_EVEN, Integer_part, Integer_part]
  */
  float pressure = readings_from_sensors[1];
  //For node A
  byte pressure_init_seq = 0xaa; //Node b = 0xba
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
  press_bin.myByte[1] = pressure_2_bytes.byte_array[0];
  press_bin.myByte[0] = pressure_2_bytes.byte_array[1];

  for(byte i=0; i<4; i++){
    sequence[i + 4 + 2] = press_bin.myByte[i];
  }
  delay(100);

//HUMITAT
/*The Packet structure for the pressure will be:      For the node A: [A5, Integer_part, Decimal_part, Decimal_part]
                                                      For the node B: [B5, Integer_Part, Decimal_part, Decimal_part]
  */
  float humidity = readings_from_sensors[1];
  //For node A
  byte humidity_init_seq = 0xa5; //node B = 0xb5
  int humidity_int = (int) humidity;
  //This byte below will be send as integer_part (it fits in a byte as it is from 20 to 90)
  byte humidity_byte = (byte) humidity_int;
  float humidity_decimal_part = humidity - humidity_int;
  //This int below will be sent as decimal part
  int humidity_decimal_part_int = (int) (decimal_part * 1000); //We get 3 decimals from the number
  int2bytes converter_humidity;
  converter_humidity.myInt = humidity_decimal_part_int;
  
  burst humidity_burst;
  humidity_burst.myByte[3] = humidity_init_seq;
  humidity_burst.myByte[2] = humidity_byte;
  humidity_burst.myByte[1] = converter_humidity.byte_array[0];
  humidity_burst.myByte[0] = converter_humidity.byte_array[1];
  for(byte i=0; i<4; i++){
    sequence[i + 8 + 2] = humidity_burst.myByte[i];
  }
  
  delay(100);
  
  //CRC computation [initseq0, initseq1, crc, 0x00]
  byte crc = compute_array_CRC(sequence);
  //We create the first burst (32 bits = 4B : initseq0(1B) + initseq1(1B) + crc(1B) + padding(1B)
  burst first;
  //[3,2,1,0]
  first.myByte[3] = initseq0;
  first.myByte[2] = initseq1;
  first.myByte[1] = crc;
  first.myByte[0] = 0x00; //Padding
  Serial.println(first.myByte[2]);
  //Send all the sequences
  Serial.println(first.myLong, HEX);
  irsend.sendNEC(first.myLong, 32);
  delay(100);
  Serial.println(temp_burst.myLong, HEX);
  irsend.sendNEC(temp_burst.myLong, 32);
  delay(100);
  Serial.println(press_bin.myLong, HEX);
  irsend.sendNEC(press_bin.myLong, 32);
  delay(100);
  Serial.println(humidity_burst.myLong, HEX);
  irsend.sendNEC(humidity_burst.myLong, 32);
  delay(500);

}
  

