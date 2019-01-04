#include <IRremote.h>
#include <IRremoteInt.h>

#define SEQ_LEN_NO_CRC 14
#define SEQ_LEN_CRC 16 //4 Bursts*4 Bytes

/*Brifly, the receiver works as follows:
 1. Receives a sequence of 4 Bytes
    1.1 If in this sequence the init sequence is present, stores this 4 Bytes in an array of SEQ_LEN. If the sequence does not contain the initsequence, it
    disscards the packet and keeps waiting for a packet with initsequence
 2. Receive another 3 sequences of 4 Bytes. 
 3. Once 4 bursts are received (4*4Bytes= 16 Bytes received), it computes the CRC of 14 of these 16 Bytes (because two bytes are CRC + padding) to check if the content is valid. 
 4. If the content is valid, it extracts the sensor readings
    */

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
/*Unions were used to easily take bytes from a float number*/
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


/*The method compute_CRC computes the number of ones in one byte*/
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
/*This method computes the number of ones in one array of bytes*/
byte compute_array_CRC(byte *arr){
  byte count = 0;
  byte len = sizeof(arr);
  for(byte i = 0; i <SEQ_LEN_NO_CRC; i++){
    count = count + compute_CRC(arr[i]);
  }
  return count;
}
  
void loop() {
  // put your main code here, to run repeatedly:
  //In sequence we will store all the bytes received
  byte sequence[SEQ_LEN_CRC];
  //in sequence_to_compute_crc we will store only the 14 Bytes that are used to compute the CRC
  byte sequence_to_compute_crc[SEQ_LEN_CRC];
  byte crc;
  bool end_of_seq = false;
  bool incorrect_crc = false;
  // index j is the index that we will update if we received a known initsequence in order to keep saving the 3 remaining packets
  byte j = 0;
  
  while(!end_of_seq || !incorrect_crc){
   
    if(irrecv.decode(&results)){
      byte first_bytes_rcv[4];
      unsigned long valor_rebut = results.value;
      burst rcv_burst;
      rcv_burst.myLong = valor_rebut;
      
      first_bytes_rcv[3] = rcv_burst.myByte[0]; //crc
      first_bytes_rcv[2] = rcv_burst.myByte[1]; //padding
      first_bytes_rcv[1] = rcv_burst.myByte[2]; //initseq1
      first_bytes_rcv[0] = rcv_burst.myByte[3]; //initseq0
      
      for(byte i=0; i<4; i++){
        sequence[i+j] = first_bytes_rcv[i];
      }
  
      if( first_bytes_rcv[0]== 0xa0 && first_bytes_rcv[1] == 0xa0){
        //Serial.println("Receiving from node A");
        // This is a known initsequence, so we update the sequence index
        crc = first_bytes_rcv[3];
        j = j + 4;
      }else if(first_bytes_rcv[0] == 0xb0 && first_bytes_rcv[1] == 0xb0){
        //Serial.println("Receiving from node B");
        //Update the sequence index as this is a known initsequence
        crc = first_bytes_rcv[3];
        j = j + 4;
      }else{
        //The received signal is nothing known by the receiver so we don't update j
        end_of_seq = true;
      }
      if(first_bytes_rcv[0]== 0xaf || first_bytes_rcv[0]== 0xa5 || first_bytes_rcv[0]== 0xaa ||first_bytes_rcv[0]== 0xbf || first_bytes_rcv[0]== 0xb5 || first_bytes_rcv[0]== 0xba || first_bytes_rcv[0]== 0xbe){
        //If we are inside this condition that means that is some code from our node so we have to keep receiving
        j = j + 4;
      }
      if(j == 16){
        /*If we are inside this condition that menas that we have received 4 bursts of 4 Bytes (16 Bytes). Now we can do the proper computations, that is:
        1- compute crc
        2- In case that this one is OK, extract information from sensors taking into account the protocol decided by the team.
        */
        //Serial.println("4 bursts received!!");
        //update the sequence_to_compute_crc content (we don't use neither the padding or the crc for obvious reasons)
        for(byte k = 0; k<2; k++){
          sequence_to_compute_crc[k] = sequence[k];
        }
        
        for(byte k = 2; k < SEQ_LEN_NO_CRC; k++){
          sequence_to_compute_crc[k] = sequence[k + 2];
        }
        byte computed_crc = compute_array_CRC(sequence_to_compute_crc);
        //Serial.println("CRC received");
        //Serial.println(crc, HEX);
        //Serial.println("CRC Computed");
        //Serial.println(computed_crc, HEX);
        if(computed_crc == crc){
          //Serial.println("CRC OK!");
          for(byte p = 4; p<16; p++){
            if(p%4 == 0){
              if(sequence[p] == 0xaf || sequence[p] == 0xbf){
                byte integer_part_temp = sequence[p+1] - 20;
                byte dec_part_temp[2];
                int part_entera_temp = (int) integer_part_temp;
                dec_part_temp[0] = sequence[p+1];
                dec_part_temp[1] = sequence[p+2];
                int2bytes temp_int2bytes;
                temp_int2bytes.byte_array[0] = dec_part_temp[0];
                temp_int2bytes.byte_array[1] = dec_part_temp[1];
                int decimals = temp_int2bytes.myInt;
                Serial.println(decimals, DEC);
                float temperatura = (float) (part_entera_temp + (decimals/1000.0));
                Serial.println(temperatura, DEC);
            }
          }
        }else{
          //Serial.println("CRC NOT OK!");
        }
        //Whether the sequence was correct or not, we must reset the j index to start receiving another sequence from j = 0
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
