//Specify supported colors
enum LED_COLOR {
  LED_COLOR_GREEN,
  LED_COLOR_BLUE,
  LED_COLOR_RED,
  LED_COLOR_CYAN,
  LED_COLOR_YELLOW,
  LED_COLOR_PURPLE,
  LED_COLOR_WHITE,
  LED_COLOR_SEQUENTIAL,
  LED_COLOR_MULTI,
  LED_COLOR_END
};

//Specify supported lighting modes
enum LED_MODE {
  LED_MODE_SCAN,
  LED_MODE_SCAN_CONSTANT,
  LED_MODE_PULSE,
  LED_MODE_HEARTBEAT,
  LED_MODE_CELLULAR_AUTOMATON,
  LED_MODE_TWINKLE,
  LED_MODE_BOOTS_AND_PANTS,
  LED_MODE_VU_METER,
  LED_MODE_PARTY_SHUFFLE,
  LED_MODE_END
};