/*
 * 3pi-mazesolver - demo code for the Pololu 3pi Robot
 * 
 * This code will solve a line maze constructed with a black line on a
 * white background, as long as there are no loops.  It has two
 * phases: first, it learns the maze, with a "left hand on the wall"
 * strategy, and computes the most efficient path to the finish.
 * Second, it follows its most efficient solution.
 *
 * http://www.pololu.com/docs/0J21
 * http://www.pololu.com
 * http://forum.pololu.com
 *
 */

// The 3pi include file must be at the beginning of any program that
// uses the Pololu AVR library and 3pi.
#include <pololu/3pi.h>

// This include file allows data to be stored in program space.  The
// ATmega168 has 16k of program space compared to 1k of RAM, so large
// pieces of static data should be stored in program space.
#include <avr/pgmspace.h>

#include "bargraph.h"
#include "maze-solve.h"
#include "sounds.h"
#include "calibrate.h"

// Introductory messages.  The "PROGMEM" identifier causes the data to
// go into program space.
const char welcome_line1[] PROGMEM = " Pololu";
const char welcome_line2[] PROGMEM = "3\xf7 Robot";
const char demo_name_line1[] PROGMEM = "Maze";
const char demo_name_line2[] PROGMEM = "solver";

// Initializes the 3pi, displays a welcome message, calibrates, and
// plays the initial music.
void initialize()
{ 
  unsigned int sensors[5]; // an array to hold sensor values
  
	// This must be called at the beginning of 3pi code, to set up the
	// sensors.  We use a value of 2000 for the timeout, which
	// corresponds to 2000*0.4 us = 0.8 ms on our 20 MHz processor.
	pololu_3pi_init(2000);
	load_custom_characters(); // load the custom characters
  
  if (!check_stored_calibration() || button_is_pressed(BUTTON_C))
    perform_calibration(); // loops forever when done
  
	play_from_program_space(welcome_sound);

	// Display battery voltage for 2 s or until button press
	while(millis() < 2000 && !button_is_pressed(BUTTON_A))
	{
		int bat = read_battery_millivolts();

		clear();
		print_long(bat);
		print("mV");
		lcd_goto_xy(0,1);
		print("Press B");

		delay_ms(100);
	}
  wait_for_button_release(BUTTON_A);

	load_stored_calibration();

	// Display calibrated values as a bar graph.
	while(!button_is_pressed(BUTTON_A))
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
	wait_for_button_release(BUTTON_A);

	clear();

	print("Go!");		

	// Play music and wait for it to finish before we start driving.
	play_from_program_space(go_sound);
	while(is_playing());
}

// This is the main function, where the code starts.  All C programs
// must have a main() function defined somewhere.
int main()
{
	// set up the 3pi
	initialize();

	// Call our maze solving routine.
	map_maze();

	// Now enter an infinite loop - we can re-run the maze as many
  // times as we want to.
  while(1)
  {    
    unsigned char button = wait_for_button(BUTTON_A | BUTTON_C);
    play_from_program_space(go_sound);
	  while (is_playing());

    if (button == BUTTON_A)
    {
      run_maze_aggressive();
    }
    else
    {
      set_digital_output(IO_D0, LOW);
      run_maze_conservative();
      set_digital_input(IO_D0, PULL_UP_ENABLED);
    } 
  }
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: t **
// end: **
