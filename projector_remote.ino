#include <IRremote.h>

const int IR_RECV_PIN = 7;
IRrecv irrecv(IR_RECV_PIN);
decode_results results;

const int LED_BLUE = 12;
const int LED_RED = 8;

/* Time to fully descend */
const int DOWN_MILLIS = 5000;
/* Time to fully ascend */
const int UP_MILLIS = 10000;

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
  UP = 0,
  DOWN,
  ASCENDING,
  DESCENDING,
  STOPPED,
} projector_state_t;

projector_state_t projector_state = UP;
/* Position (in millis down) */
int cur_pos = 0;

unsigned long last_action_time;
unsigned long end_time;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT); /* Internal pull-up/op-amp turns LED on
                                 * otherwise */

  stop();

  irrecv.enableIRIn();
  irrecv.blink13(false);

  Serial.println(F("Done setting up"));
}

// https://www.circuitbasics.com/arduino-ir-remote-receiver-tutorial/
// Driving relays via active high: https://forum.arduino.cc/index.php?topic=274215.15

void loop() {
  // put your main code here, to run repeatedly:
  if (irrecv.decode(&results)) {
    button_t button = decode_ir(results.value);
    if (button != -1) {
      Serial.println(button);
    }

    switch (button) {
    case MINUS:
      handle_down_press();
      break;
    case PLUS:
      handle_up_press();
      break;
    case CH_D:
      /* TODO: reset DOWN position */
      break;
    case CH_U:
      /* TODO: reset UP position */
      break;
    case EQ:
      stop();
      break;
    }
    irrecv.resume();
   }

  switch(projector_state) {
  case UP:
  case DOWN:
    /* Nothing to do */
    break;
  case DESCENDING:
    /* TODO: handle overflow */
    if (millis() >= end_time) {
      /* All done */
      stop();
      projector_state = DOWN;
      cur_pos = DOWN_MILLIS;
      Serial.println(F("Done"));
    }
    break;
  case ASCENDING:
    /* TODO: handle overflow */
    if (millis() >= end_time) {
      /* All done */
      stop();
      projector_state = UP;
      cur_pos = 0;
      Serial.println(F("Done"));
    }
    break;
  }

  /* find_current_position(); */
  /* Serial.print("Cur pos: "); */
  /* Serial.println(cur_pos); */
}

void handle_down_press() {
  switch (projector_state) {
  case DOWN:
  case DESCENDING:
    /* Already down/descending, do nothing */
    Serial.println(F("Already down or descending, ignoring"));
    break;
  case UP:
    /* Start descending for DOWN_MILLIS milliseconds */
    Serial.println(F("Going down"));
    descend(DOWN_MILLIS);
    break;
  case ASCENDING:
  case STOPPED:
    /* Scale how long we ascended into an equivalent amount of (negative) down
     * millis, use that to update our position, and use that to find how much
     * time remains to full descent. */
    find_current_position();

    unsigned int down_time = DOWN_MILLIS - cur_pos;
    report_current_position();
    Serial.print("Going down for only ");
    Serial.print(down_time);
    Serial.println(F(" millis"));
    descend(down_time);
    break;
  }
}

void handle_up_press() {
  switch (projector_state) {
  case UP:
  case ASCENDING:
    /* Already up/ascending, do nothing */
    Serial.print(F("Already up or ascending (cur pos "));
    Serial.print(cur_pos);      /* TODO: */
    Serial.println(F("), ignoring"));
    break;
  case DOWN:
    /* Start ascending for UP_MILLIS milliseconds */
    Serial.println(F("Going up"));
    ascend(UP_MILLIS);
    break;
  case DESCENDING:
  case STOPPED:
    /* Our speed is linear, so increase our current position based on how long
     * we descended, then scale that to UP_MILLIS to find how long to ascend */
    find_current_position();

    unsigned int up_time = map(cur_pos, 0, DOWN_MILLIS, 0, UP_MILLIS);
    report_current_position();
    Serial.print(F("Going up for only "));
    Serial.print(up_time);
    Serial.println(F(" millis"));
    ascend(up_time);
    break;
  }
}

void find_current_position() {
  /* Update the cur_pos variable, as well as possibly last_action_time */
  unsigned long now;
  switch(projector_state) {
  case UP:
    cur_pos = 0;
    break;
  case DOWN:
    cur_pos = DOWN_MILLIS;
    break;
  case DESCENDING:
    now = millis();
    cur_pos += now - last_action_time;
    last_action_time = now;
    break;
  case ASCENDING:
    now = millis();
    cur_pos -= map(now - last_action_time, 0, UP_MILLIS, 0, DOWN_MILLIS);
    last_action_time = now;
    break;
  case STOPPED:
    /* Nothing we can do here, so hope our position was set correctly when we
     * stopped. */
    break;
  }
}

void report_current_position() {
    Serial.print(F("Current position "));
    Serial.println(cur_pos);
}

void stop() {
  disable_down();
  disable_up();
  find_current_position();
  projector_state = STOPPED;
  report_current_position();
}

void descend(unsigned long ms) {
  unsigned long now;

  projector_state = DESCENDING;
  disable_up();
  enable_down();
  now = millis();
  end_time = now + ms;
  last_action_time = now;
}

void ascend(unsigned long ms) {
  unsigned long now;

  projector_state = ASCENDING;
  disable_down();
  enable_up();
  now = millis();
  end_time = now + ms;
  last_action_time = now;
}

void enable_down() {
  digitalWrite(LED_BLUE, HIGH);
}

void disable_down() {
  digitalWrite(LED_BLUE, LOW);
}

void enable_up() {
  digitalWrite(LED_RED, HIGH);
}

void disable_up() {
  digitalWrite(LED_RED, LOW);
}

button_t decode_ir(unsigned long value) {
  for (int i=CH_D; i<=D9; i++) {
    if (value == car_mp3_codes[i]) {
      return (button_t)i;
    }
  }
  return -1;
}
