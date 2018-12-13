# HUB EIO
This project consist in sending different sensor measurements through an IR link using library IRremote for arduino.
## Fields

Node_ids = {A, B} (Hexadecimal)

Sensor_ids = {0x0: Initial Burst, 
              0xF: Temperature, 
              0xA: Pressure, 
              0x5: Humidity, 
              0xE: Luminosity} 
              

Padding = 0xEE

## Packet Structure

Initial_burst     = [ [node_id, sensor_id] [node_id, sensor_id] [Padding] [CRC] ]  ---> 32 bits = 4 Bytes

Temperature Burst = [ [node_id, sensor_id] [integer_part_of_temperature + 20] [decimal_part_1] [decimal_part_2] ]


Pressure Burst    = [ [node_id, sensor_id] [ODD_EVEN] [integer_part_dividedby2_1] [integer_part_dividedby2_2] ]

                  ODD_EVEN = 0xff if the number is odd, 0x00 if the number is even

Humidity Burst    = [ [node_id, sensor_id] [integer_part_of_humidity] [decimal_part_1] [decimal_part_2] ]

Luminosity Burst  = [ [node_id, sensor_id] [ODD_EVEN] [integer_part_dividedby2_1] [integer_part_dividedby2_2] ]

## Burst Examples

### Temperature
For a temperature in the node B of float temp = 10.41 we will have a Temperature Burst of: [ [0xBF] [0x1E] [0x01] [0x99] ] 

To get the temperature from this burst, one must do:
    1. Get integer part and substract 20:   0x1E = 30 --> 30 - 20 = 10
    2. Get decimal part, divide it by 1000 (to get decimals) and add it to the integer part:  
