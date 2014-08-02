#ifndef __sounds_h
#define __sounds_h

extern const char welcome_sound[] PROGMEM;
extern const char calibrate_welcome_sound[] PROGMEM;
extern const char calibrate_done_sound[] PROGMEM;
extern const char go_sound[] PROGMEM;
extern const char done_sound[] PROGMEM;
extern const char map_turn_sound[] PROGMEM;
extern const char run_turn_sound[] PROGMEM;

void move_sound();

void delay_with_move_sound(unsigned int milliseconds);

#endif