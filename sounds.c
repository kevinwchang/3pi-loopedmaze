#include <avr/pgmspace.h>

const char welcome_sound[] PROGMEM = ">g32>>c32";
const char calibrate_welcome_sound[] PROGMEM = "g16>c16";
const char calibrate_done_sound[] PROGMEM = ">c16g16";
//const char go_sound[] PROGMEM = "!A";
const char go_sound[] PROGMEM = "!O5L16MS c#>c#g#f ML>c#32MSg#.f8 d>daf# ML>d32MSa.f#8 c#>c#g#f ML>c#32MSg#.f8 MLg32f#32MSg MLg32g#32MSa MLa32b-32MSb>c#8";
const char done_sound[] PROGMEM = "!T90L32 f#.r64f#.r64f#.r64d#c# f#64.r128f#16a#32a#8 f#.r64f#.r64f#.r64d#c# f#64.r128f#16d#32d#8 f#.r64f#.r64f#.r64d#c# f#64.r128f#16a32L16ab >cbaf#a.f#32f#8";
const char map_turn_sound[] PROGMEM = "!T4337 O3eg#O4ceg#a# T2891 r2. afc#<a";
const char run_turn_sound[] PROGMEM = "!T2940O4c#1f#2.a#2O5c#.d#ff#gg#abO6cc#dd#eff#";