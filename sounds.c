#include <pololu/3pi.h>

const char welcome[] PROGMEM = ">g32>>c32";
const char calibrate_welcome[] PROGMEM = "g16>c16";
const char calibrate_done[] PROGMEM = ">c16g16";
//const char go[] PROGMEM = "!A";
const char go[] PROGMEM = "!O5L16MS c#>c#g#f ML>c#32MSg#.f8 d>daf# ML>d32MSa.f#8 c#>c#g#f ML>c#32MSg#.f8 MLg32f#32MSg MLg32g#32MSa MLa32b-32MSb>c#8";
const char done[] PROGMEM = "!T90L32 f#.r64f#.r64f#.r64d#c# f#64.r128f#16a#32a#8 f#.r64f#.r64f#.r64d#c# f#64.r128f#16d#32d#8 f#.r64f#.r64f#.r64d#c# f#64.r128f#16a32L16ab >cbaf#a.f#32f#8";
const char wakka[] PROGMEM = "!T4337 O3eg#O4ceg#a# T2891 r2. afc#<a";

/*void play_move_sound()
{
  unsigned int m = millis();
  unsigned int freq = (m << 2) % 1320;

  if (freq > 660)
  freq = 1660 - freq;
  else
  freq = 340 + freq;
  
  play_frequency(freq, 1, 10);
}

void delay_with_move_sound(unsigned int milliseconds)
{
  unsigned int startMs = millis();
  
  while (((unsigned int)millis() - startMs) < milliseconds)
  {
    if (!is_playing())
      play_move_sound();
  }
}*/