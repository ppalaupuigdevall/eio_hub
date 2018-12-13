#include <IRremote.h>
#include <IRremoteInt.h>
/*TODO:
    Hay que hacer que el receiver haga un check de las init sequence y compruebe crc
*/

#define SEQ_LEN_NO_CRC 14
#define SEQ_LEN_CRC 16 //4 Bursts*4 Bytes
int RECV_PIN = 5;
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup() { 
  // put your setup code here, to run once:
  Serial.begin(9600);
  irrecv.enableIRIn();
}

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
  // put your main code here, to run repeatedly:
  byte sequence[SEQ_LEN_CRC];
  byte sequence_to_compute_crc[SEQ_LEN_CRC];
  byte crc;
  bool end_of_seq = false;
  bool incorrect_crc = false;
  byte j = 0;
  
  while(!end_of_seq || !incorrect_crc){
   
    if(irrecv.decode(&results)){
      byte first_bytes_rcv[4];
      unsigned long valor_rebut = results.value;
      burst rcv_burst;
      rcv_burst.myLong = valor_rebut;
      
      first_bytes_rcv[3] = rcv_burst.myByte[0]; //initseq0
      first_bytes_rcv[2] = rcv_burst.myByte[1]; //initseq1
      first_bytes_rcv[0] = rcv_burst.myByte[2]; //crc
      first_bytes_rcv[1] = rcv_burst.myByte[3]; //padding

      Serial.print(first_bytes_rcv[0], HEX);
      Serial.print(first_bytes_rcv[1], HEX);
      Serial.print(first_bytes_rcv[2], HEX);
      Serial.print(first_bytes_rcv[3], HEX);
      Serial.println("");
      for(byte i=0; i<4; i++){
        sequence[i+j] = first_bytes_rcv[i];
      }
  
      if( first_bytes_rcv[3]== 0xa0 && first_bytes_rcv[2] == 0xa0){
        Serial.println("Rebo del node A");
        //Update the sequence index
        crc = first_bytes_rcv[1];
        j = j + 4;
      }else if(first_bytes_rcv[3] == 0xb0 && first_bytes_rcv[2] == 0xb0){
        Serial.println("Rebo del node B");
        //Update the sequence index
        crc = first_bytes_rcv[1];
        j = j + 4;
      }else{
        //The received signal is nothing known by the receiver
        end_of_seq = true;
      }
      if(first_bytes_rcv[2]== 0xaf || first_bytes_rcv[2]== 0xa5 || first_bytes_rcv[2]== 0xaa){
        j = j +4;
      }
      if(j == 16){
        //Aixo vol dir que he rebut tota la sequencia (15 Bytes), ara ja puc fer els calculs que toquin: 1 - calcular crc i en cas que sigui OK treure info de sensors i enviar-la a josep
        Serial.println("He rebut 4 paquets!!");
        for(byte k = 0; k<2; k++){
          sequence_to_compute_crc[k] = sequence[k];
        }
        
        for(byte k = 2; k < SEQ_LEN_NO_CRC - 2; k++){
          sequence_to_compute_crc[k] = sequence[k + 2];
        }
        byte computed_crc = compute_array_CRC(sequence_to_compute_crc);
        Serial.println("CRC rebut");
        Serial.println(crc, HEX);
        Serial.println("CRC Calculat");
        Serial.println(computed_crc, HEX);
        if(computed_crc == crc){
          Serial.println("CRC OK!");
        }else{
          Serial.println("CRC NOT OK!");
        }
        //Passi el que passi a dalt hem de sortir del while i seguir llegint
        end_of_seq = true;
        j = 0;
      }

      
   //Receive next packet
    irrecv.resume();
    delay(100);
    }
     
  }
delay(50);
}
