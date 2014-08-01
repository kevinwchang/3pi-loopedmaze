#ifndef __sounds_h
#define __sounds_h

extern const char welcome[] PROGMEM;
extern const char calibrate_welcome[] PROGMEM;
extern const char calibrate_done[] PROGMEM;
extern const char go[] PROGMEM;
extern const char done[] PROGMEM;
extern const char wakka[] PROGMEM;

void move_sound();

void delay_with_move_sound(unsigned int milliseconds);

#endif