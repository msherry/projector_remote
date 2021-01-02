#include <IRremote.h>

#define DEBUG_ENABLE
#include "DebugUtils.h"

const int IR_RECV_PIN = 7;
IRrecv irrecv(IR_RECV_PIN);
decode_results results;

const int PIN_DOWN_OUTPUT = 11;
const int PIN_UP_OUTPUT = 8;

const int PIN_DOWN_BUTTON = 4;
const int PIN_UP_BUTTON = 6;

/* Time to fully descend */
const int DOWN_MILLIS = 31015;
/* Time to fully ascend */
const int UP_MILLIS = 31685;

/* Time between switching directions */
const int RELAY_SWITCH_DELAY = 200;

/* Generic "Car MP3" remote */
unsigned long car_mp3_codes[] = {
  0xFFA25D,
  0xFF629D,
  0xFFE21D,
  0xFF22DD,
  0xFF02FD,
  0xFFC23D,
  0xFFE01F,
  0xFFA857,
  0xFF906F,
  0xFF6897,
  0xFF9867,
  0xFFB04F,
  0xFF30CF,
  0xFF18E7,
  0xFF7A85,
  0xFF10EF,
  0xFF38C7,
  0xFF5AA5,
  0xFF42BD,
  0xFF4AB5,
  0xFF52AD,
};

typedef enum button_t {
  CH_D = 0,
  CH,
  CH_U,
  BAK,
  FWD,
  PAUSE,
  MINUS,
  PLUS,
  EQ,
  D0,
  D100,
  D200,
  D1,
  D2,
  D3,
  D4,
  D5,
  D6,
  D7,
  D8,
  D9,
} button_t;


typedef enum  {
  STOPPED = 0,
  ASCENDING,
  DESCENDING,
} screen_state_t;

screen_state_t screen_state = STOPPED;
/* Position (in millis down) */
int cur_pos = 0;

unsigned long last_action_time;
unsigned long end_time;

/* Relays are active low without supporting circuitry */
#define ACTIVE_LOW

#ifdef ACTIVE_LOW
#define ENABLED LOW
#define DISABLED HIGH
#else
#define ENABLED HIGH
#define DISABLED LOW
#endif


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Disable before setting pins to output, per
  // https://arduino-info.wikispaces.com/ArduinoPower
  disable_down();
  disable_up();

  pinMode(PIN_DOWN_OUTPUT, OUTPUT);
  pinMode(PIN_UP_OUTPUT, OUTPUT);
  pinMode(PIN_DOWN_BUTTON, INPUT_PULLUP); /* reads HIGH when switch is open */
  pinMode(PIN_UP_BUTTON, INPUT_PULLUP); /* reads HIGH when switch is open */
  pinMode(LED_BUILTIN, OUTPUT); /* Internal pull-up/op-amp turns LED on
                                 * otherwise */

  irrecv.enableIRIn();
  irrecv.blink13(false);

  DEBUGLN(F("Done setting up"));
}

// https://www.circuitbasics.com/arduino-ir-remote-receiver-tutorial/
// Driving relays via active high: https://forum.arduino.cc/index.php?topic=274215.15

void loop() {
  // put your main code here, to run repeatedly:
  int did_get_input = 0;

  did_get_input = handle_physical_input();
  if (!did_get_input) {
    handle_remote_input();
  }

  switch(screen_state) {
  case STOPPED:
    /* Nothing to do */
    break;
  case DESCENDING:
  case ASCENDING:
    /* TODO: handle overflow */
    if (millis() >= end_time) {
      /* All done */
      screen_state_t old_state = screen_state; /* stop() clobbers screen_state */
      stop();
      cur_pos = (old_state == DESCENDING) ? DOWN_MILLIS : 0;
      DEBUGLN(F("Done"));
    }
    break;
  }
  /* find_current_position(); */
  /* report_current_position(); */
}

/* Check for and handle remote input */
int handle_remote_input() {
  int did_get_input = 0;

  if (irrecv.decode(&results)) {
    button_t button = decode_ir(results.value);
    if (button != -1) {
      DEBUG(F("BTN: "));
      DEBUGLN(button);
    }

    /* Serial.println(results.value, HEX); */

    switch (button) {
    case MINUS:
      handle_down_press();
      did_get_input = 1;
      break;
    case PLUS:
      handle_up_press();
      did_get_input = 1;
      break;
    case CH_D:
      handle_chd_press();
      did_get_input = 1;
      break;
    case CH_U:
      handle_chu_press();
      did_get_input = 1;
      break;
    case EQ:
      handle_eq_press();
      did_get_input = 1;
      break;
    }
    irrecv.resume();
   }
  return did_get_input;
}

int handle_physical_input() {
  int did_get_input = 0;

  /* Pins are pulled up, so interpret LOW as button pressed */
  int val = digitalRead(PIN_DOWN_BUTTON);
  if (val == LOW) {
    handle_down_press();
    did_get_input = 1;
  } else {
    /* Only attempt to read the second button when the first has no input */
    val = digitalRead(PIN_UP_BUTTON);
    if (val == LOW) {
      handle_up_press();
      did_get_input = 1;
    }
  }
  return did_get_input;
}

/* Attempt to lower the screen to the bottom */
void handle_down_press() {
  switch (screen_state) {
  case DESCENDING:
    /* Already descending, do nothing */
    DEBUGLN(F("Already descending, ignoring"));
    break;
  case ASCENDING:
  case STOPPED:
    /* Find how much further we have to descend, based on our current
     * position */
    find_current_position();
    if (cur_pos == DOWN_MILLIS) {
      DEBUGLN(F("Already down, ignoring"));
      break;
    }

    unsigned int down_time = DOWN_MILLIS - cur_pos;
    report_current_position();
    DEBUG(F("Going down for "));
    if (down_time < DOWN_MILLIS) {
      DEBUG(F("only "));
    }
    DEBUG(down_time);
    DEBUGLN(F(" millis"));
    descend(down_time);
    break;
  }
}

/* Attempt to raise the screen to the top */
void handle_up_press() {
  switch (screen_state) {
  case ASCENDING:
    /* Already ascending, do nothing */
    DEBUGLN(F("Already ascending, ignoring"));
    break;
  case DESCENDING:
  case STOPPED:
    /* Find how much further we have to ascend, based on our current
     * position (and scaled to up time) */
    find_current_position();
    if (cur_pos == 0) {
      DEBUGLN(F("Already up, ignoring"));
      break;
    }

    unsigned int up_time = map(cur_pos, 0, DOWN_MILLIS, 0, UP_MILLIS);
    report_current_position();
    DEBUG(F("Going up for "));
    if (up_time < UP_MILLIS) {
      DEBUG(F("only "));
    }
    DEBUG(up_time);
    DEBUGLN(F(" millis"));
    ascend(up_time);
    break;
  }
}

/* Reset the screen to the down position */
void handle_chd_press() {
  if (screen_state != STOPPED) {
    DEBUGLN(F("Screen is not stopped, not resetting position"));
    return;
  }
  DEBUGLN(F("Resetting screen to DOWN position"));
  cur_pos = DOWN_MILLIS;
  report_current_position();
}

/* Reset the screen to the up position */
void handle_chu_press() {
  if (screen_state != STOPPED) {
    DEBUGLN(F("Screen is not stopped, not resetting position"));
    return;
  }
  DEBUGLN(F("Resetting screen to UP position"));
  cur_pos = 0;
  report_current_position();
}

/* Stop the screen where it is */
void handle_eq_press() {
  stop();
}

void find_current_position() {
  /* Update the current position, as well as last_action_time */
  unsigned long now = millis();

  switch(screen_state) {
  case DESCENDING:
    cur_pos += now - last_action_time;
    break;
  case ASCENDING:
    cur_pos -= map(now - last_action_time, 0, UP_MILLIS, 0, DOWN_MILLIS);
    break;
  case STOPPED:
    /* Nothing we can do here, so hope our position was set correctly when we
     * stopped. */
    break;
  }
  last_action_time = now;
}

void report_current_position() {
    DEBUG(F("Current position (0-100): "));
    DEBUGLN(map(cur_pos, 0, DOWN_MILLIS, 0, 100));
}

void stop() {
  disable_down();
  disable_up();
  find_current_position();
  screen_state = STOPPED;
  report_current_position();
}

void descend(unsigned long ms) {
  unsigned long now;

  screen_state = DESCENDING;
  disable_up();
  delay(RELAY_SWITCH_DELAY);
  enable_down();
  now = millis();
  end_time = now + ms;
  last_action_time = now;
}

void ascend(unsigned long ms) {
  unsigned long now;

  screen_state = ASCENDING;
  disable_down();
  delay(RELAY_SWITCH_DELAY);
  enable_up();
  now = millis();
  end_time = now + ms;
  last_action_time = now;
}

void enable_down() {
  digitalWrite(PIN_DOWN_OUTPUT, ENABLED);
}

void disable_down() {
  digitalWrite(PIN_DOWN_OUTPUT, DISABLED);
}

void enable_up() {
  digitalWrite(PIN_UP_OUTPUT, ENABLED);
}

void disable_up() {
  digitalWrite(PIN_UP_OUTPUT, DISABLED);
}

button_t decode_ir(unsigned long value) {
  for (int i=CH_D; i<=D9; i++) {
    if (value == car_mp3_codes[i]) {
      return (button_t)i;
    }
  }
  return -1;
}
