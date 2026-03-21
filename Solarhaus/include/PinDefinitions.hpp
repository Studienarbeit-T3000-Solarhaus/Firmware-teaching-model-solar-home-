// Pin definitions for the Xiao Esp32-C3   
//                     ______________
//                   |   [USB-C]     |
//                   |               |
//   (A0) GPIO 2  D0-| O           O |- 5V
//                   |               |
//   (A1) GPIO 3  D1-| O           O |- GND
//                   |               |
//   (A2) GPIO 4  D2-| O           O |- 3V3
//                   |               |
//        GPIO 5  D3-| O           O |- D10  GPIO 10 (MOSI)
//                   |               |
//  (SDA) GPIO 6  D4-| O           O |- D9   GPIO 9  (MISO)
//                   |               |
//  (SCL) GPIO 7  D5-| O           O |- D8   GPIO 8  (SCK)
//                   |               |
//  (TX)  GPIO 21 D6-| O           O |- D7   GPIO 20 (RX)
//                   |_______________|

//            _____________________
//           |      MCP23017       |
//           |    _           _    |
//     GPB0 -| 1 |U|         28 |- GPA7
//     GPB1 -| 2             27 |- GPA6
//     GPB2 -| 3             26 |- GPA5
//     GPB3 -| 4             25 |- GPA4
//     GPB4 -| 5             24 |- GPA3
//     GPB5 -| 6             23 |- GPA2
//     GPB6 -| 7             22 |- GPA1
//     GPB7 -| 8             21 |- GPA0
//      VDD -| 9             20 |- INTA
//      VSS -| 10            19 |- INTB
//       NC -| 11            18 |- /RESET
//      SCK -| 12            17 |- A2
//      SDA -| 13            16 |- A1
//       NC -| 14            15 |- A0
//           |_____________________|



// Mosfet control pins connected to MCP23017 GPIOs
#define SOLAR_CELL_1 0
#define SOLAR_CELL_2 1
#define SOLAR_CELL_3 2
#define SOLAR_CELL_4 3
#define ENABLE_BUCK_BOOST_CONVERTER 4
#define ENABLE_MPPT 5
#define CAPACITOR_3 6
#define CAPACITOR_4 7
#define HEAVY_LOAD 8 
#define NIGHT_LOAD 9
#define CONSTANT_LOAD 10

// Definition of the pins for Neopixel LED strip
#define NEOPIXEL_PIN 4


// Wakeup pin for deep sleep
#define WAKEUP_PIN 2

// Battery voltage measurement pin 
#define BATTERY_VOLTAGE_PIN 3

// Enable 3V3 Pin
#define ENABLE_3V3_PIN 20

// Enable Battery Pin
#define ENABLE_BATTERY_PIN 21 

// I2C Pins 
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7

// MPPT Pin
#define MPPT_PWM_PIN 8
