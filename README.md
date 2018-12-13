# HUB EIO
This project consists in sending different sensor measurements through an IR link using the library IRremote for arduino.

Project members: M.Mas, P.Palau, A.Scherk, J.Torné

## Fields

Node_ids = {A, B} (Hexadecimal)

Sensor_ids = {0x0: Initial Burst, 
              0xF: Temperature, 
              0xA: Pressure, 
              0x5: Humidity, 
              0xE: Luminosity} 
              
Padding = 0xEE
CRC = Sum of all the ones in the first two bytes of the initial burst and the 3 sensor bursts. 
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
    2. Get decimal part, divide it by 1000 (to get decimals) and add it to the integer part: 0x0199 = 409 --> 409/1000 = 0.409 
    3. Add it to the integer part to recover the original temperature: 10.409 (as we need a resolution of 0.5 we are safe)
    
### Pressure
For a pressure in the node B of float press = 109999.2 Pa we will have a Pressure Burst of: [ [0xBA] [0xFF] [0xD6] [0xD7] ]

To get the pressure from this burst, one must do:
    
    1. Get integer part: 0xD6D7 = 54999 
    2. Multiply it by 2: 54999 * 2 = 109998
    3. If ODD_EVEN == 0xFF, add 1 to the number, otherwise left as it is. In this case we must add 1, resulting in: 109998+1 = 109999


