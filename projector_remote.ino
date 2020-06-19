#include <IRremote.h>

const int IR_RECV_PIN = 7;
IRrecv irrecv(IR_RECV_PIN);
decode_results results;

const int LED_BLUE = 13;
const int LED_RED = 12;

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

  irrecv.enableIRIn();
  /* irrecv.blink13(true); */

  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);


  Serial.println("Done setting up");
}

// https://www.circuitbasics.com/arduino-ir-remote-receiver-tutorial/

void loop() {
  // put your main code here, to run repeatedly:
  if (irrecv.decode(&results)) {
    /* Serial.print(results.decode_type); Serial.print("  "); */
    /* Serial.println(results.value, HEX); */

    button_t button = decode_ir(results.value);
    if (button != -1) {
      Serial.println(button);
    }

    switch (button) {
    case MINUS:
      handle_down();
      break;
    case PLUS:
      handle_up();
      break;
    }

    irrecv.resume();
   }
}

void handle_down() {
  digitalWrite(LED_BLUE, HIGH);
  delay(1000);
  digitalWrite(LED_BLUE, LOW);
}

void handle_up() {
  digitalWrite(LED_RED, HIGH);
  delay(1000);
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
