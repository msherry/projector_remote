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

unsigned long start_time;
unsigned long end_time;

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
} projector_state_t;

projector_state_t projector_state = UP;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  stop();

  irrecv.enableIRIn();

  Serial.println(F("Done setting up"));
}

// https://www.circuitbasics.com/arduino-ir-remote-receiver-tutorial/

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
      /* TODO: stop */
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
    }
    break;
  case ASCENDING:
    /* TODO: handle overflow */
    if (millis() >= end_time) {
      /* All done */
      stop();
      projector_state = UP;
    }
    break;
  }
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
    /* Our speed is linear, so map how far up we are based on UP_MILLIS to the
     * amount of DOWN_MILLIS it will take to reverse it */

    /* TODO: this only counts time since last transition, so rapid alternation
     * between up/down will break it. We should instead store our progress
     * up/down at each transition and base off of that */
    unsigned int up_time = millis() - start_time;
    unsigned int down_time = map(up_time, 0, UP_MILLIS, 0, DOWN_MILLIS);
    Serial.print(F("Already ascended for "));
    Serial.print(up_time);
    Serial.print(" millis, going down for only ");
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
    Serial.println(F("Already up or ascending, ignoring"));
    break;
  case DOWN:
    /* Start ascending for UP_MILLIS milliseconds */
    Serial.println(F("Going up"));
    ascend(UP_MILLIS);
    break;
  case DESCENDING:
    /* Our speed is linear, so map how far down we are based on DOWN_MILLIS to the
     * amount of UP_MILLIS it will take to reverse it */
    unsigned int down_time = millis() - start_time;
    unsigned int up_time = map(down_time, 0, DOWN_MILLIS, 0, UP_MILLIS);
    Serial.print(F("Already descended for "));
    Serial.print(down_time);
    Serial.print(" millis, going up for only ");
    Serial.print(up_time);
    Serial.println(F(" millis"));
    ascend(down_time);
    break;
  }
}

void stop() {
  disable_down();
  disable_up();
}

void descend(unsigned long ms) {
  unsigned long now;

  projector_state = DESCENDING;
  disable_up();
  enable_down();
  now = millis();
  end_time = now + ms;
  start_time = now;
}

void ascend(unsigned long ms) {
  unsigned long now;

  projector_state = ASCENDING;
  disable_down();
  enable_up();
  now = millis();
  end_time = now + ms;
  start_time = now;
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
