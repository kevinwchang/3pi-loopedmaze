#include "calibrate.h"

#include <pololu/3pi.h>
#include <avr/eeprom.h>
#include "sounds.h"
#include "bargraph.h"

#define LINE_SENSOR_COUNT 5

unsigned int EEMEM stored_minimum_on[LINE_SENSOR_COUNT];
unsigned int EEMEM stored_maximum_on[LINE_SENSOR_COUNT];


void save_stored_calibration()
{ 
  for (uint8_t i = 0; i < LINE_SENSOR_COUNT; i++)
  {
    eeprom_write_word(&stored_minimum_on[i], get_line_sensors_calibrated_minimum_on()[i]);
    eeprom_write_word(&stored_maximum_on[i], get_line_sensors_calibrated_maximum_on()[i]);
  }
}

void perform_calibration()
{
  unsigned int counter; // used as a simple timer
  unsigned int sensors[5]; // an array to hold sensor values
  
  play("g16>c16");
  clear();
  print("Calibr8");
  lcd_goto_xy(0,1);
  print("Press B");
      
  wait_for_button(BUTTON_B);
  
  // Wait 1 second and then begin automatic sensor calibration
  // by rotating in place to sweep the sensors over the line
  delay_ms(1000);
  
  // Auto-calibration: turn right and left while calibrating the
  // sensors.
  for(counter=0;counter<80;counter++)
  {
    if(counter < 20 || counter >= 60)
    set_motors(40,-40);
    else
    set_motors(-40,40);

    // This function records a set of sensor readings and keeps
    // track of the minimum and maximum values encountered.  The
    // IR_EMITTERS_ON argument means that the IR LEDs will be
    // turned on during the reading, which is usually what you
    // want.
    calibrate_line_sensors(IR_EMITTERS_ON);

    // Since our counter runs to 80, the total delay will be
    // 80*20 = 1600 ms.
    delay_ms(20);
  }
  set_motors(0,0);
  
  save_stored_calibration();

  // Display calibrated values as a bar graph.
  while(1)
  {
    // Read the sensor values and get the position measurement.
    unsigned int position = read_line(sensors,IR_EMITTERS_ON);

    // Display the position measurement, which will go from 0
    // (when the leftmost sensor is over the line) to 4000 (when
    // the rightmost sensor is over the line) on the 3pi, along
    // with a bar graph of the sensor readings.  This allows you
    // to make sure the robot is ready to go.
    clear();
    print_long(position);
    lcd_goto_xy(0,1);
    display_readings(sensors);

    delay_ms(100);
  }
}

bool check_stored_calibration()
{
  return (eeprom_read_word(&stored_maximum_on[LINE_SENSOR_COUNT - 1]) != 0xFFFF);
}

void load_stored_calibration()
{
  calibrate_line_sensors(IR_EMITTERS_ON); // allocate stuff
  
  for (uint8_t i = 0; i < LINE_SENSOR_COUNT; i++)
  {
    get_line_sensors_calibrated_minimum_on()[i] = eeprom_read_word(&stored_minimum_on[i]);
    get_line_sensors_calibrated_maximum_on()[i] = eeprom_read_word(&stored_maximum_on[i]);
  }
}