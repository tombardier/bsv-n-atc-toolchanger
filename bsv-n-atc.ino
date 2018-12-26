//Constants

#define TURRET_MOVE_PIN 2
#define STROBE_SIGNAL_STATUS_PIN 3
#define INDEXING_CONTROL_SWITCH_PIN 5  // Double-check this
#define LOCKING_CONTROL_SWITCH_PIN 6  // and this one

#define INDEXING_SOLENOID_PIN 23
#define TURRET_CLOCKWISE_RUN_PIN 24
#define TURRET_COUNTER_CLOCKWISE_RUN_PIN 25

#define STROBE_SIGNAL_MARGIN_MS 10  // In case of an intermittent signal, make sure it's safely past
#define ENGAGE_LOCKING_PIN_MAX_LAG 40  // Time "R1" in docs
#define REACT_LOCKING_CONTROL_SWITH_MAX_LAG 40  // Time "R2" in docs
#define TIME_BETWEEN_MOTOR_DIRECTION_REVERSAL 50  // Time "T1" in docs
#define TIME_TO_DISENGAGE_LOCKING_PIN 200  // Time "T2" in docs


enum instructions {
    READY_TO_START_RUN_CYCLE,
    START_CLOCKWISE_MOVEMENT,
    WAIT_FOR_STROBE_SIGNAL_LOW_ENERGISE_LOCKING_PIN,
    WAIT_FOR_INDEX_CONTROL_SWITCH_AND_STOP_MOTOR,
    WAIT_THEN_REVERSE_MOTOR,
    WAIT_FOR_LOCKING_CONTROL_SWITCH_AND_STOP_MOTOR,
    WAIT_THEN_DISENGAGE_LOCKING_PIN,
    END_RUN_CYCLE_RESET_STATE,
    ERROR_STATE
};


// State variables
enum instructions next_instruction = READY_TO_START_RUN_CYCLE;
unsigned long time_since_last_state_transition = 0;

void setup() {
    pinMode(TURRET_MOVE_PIN, INPUT); // Turret Move
    pinMode(STROBE_SIGNAL_STATUS_PIN, INPUT); // Strobe Signal Status
    pinMode(INDEXING_CONTROL_SWITCH_PIN, INPUT); // Indexing Control Switch - 5
    pinMode(LOCKING_CONTROL_SWITCH_PIN, INPUT); // Locking Control Switch - 6

    pinMode(INDEXING_SOLENOID_PIN, OUTPUT); // Indexing Solenoid
    pinMode(TURRET_CLOCKWISE_RUN_PIN, OUTPUT); // Turret Clockwise Run
    pinMode(TURRET_COUNTER_CLOCKWISE_RUN_PIN, OUTPUT); // Turret Counter Clockwise Run
}

 
void stop_and_disengage_everything() {
    digitalWrite(TURRET_CLOCKWISE_RUN_PIN, LOW);
    digitalWrite(TURRET_COUNTER_CLOCKWISE_RUN_PIN, LOW);
    digitalWrite(INDEXING_SOLENOID_PIN, LOW);
}


void loop() {
    switch (next_instruction) {

        // If ready to start, check for the turret move pin, which is the trigger to start a run cycle.
        // Firstly, reset the state of all pins to a known value, and move on to start clockwise movement
        case READY_TO_START_RUN_CYCLE:
            if (digitalRead(TURRET_MOVE_PIN) == HIGH) {
                digitalWrite(TURRET_CLOCKWISE_RUN_PIN, LOW);
                digitalWrite(TURRET_COUNTER_CLOCKWISE_RUN_PIN, LOW);
                digitalWrite(INDEXING_SOLENOID_PIN, LOW);
                next_instruction = START_CLOCKWISE_MOVEMENT;
                time_since_last_state_transition = millis();
            }
            
        // Begin moving clockwise.
        case START_CLOCKWISE_MOVEMENT:
            digitalWrite(TURRET_CLOCKWISE_RUN_PIN, HIGH);
            next_instruction = WAIT_FOR_STROBE_SIGNAL_LOW_ENERGISE_LOCKING_PIN;
            time_since_last_state_transition = millis();

            
        // While moving clockwise, check for the strobe to be passed, by a sensible margin.
        // This indicates the starting tool position index has been passed
        // Once this is the case, start to engage the locking pin
        case WAIT_FOR_STROBE_SIGNAL_LOW_ENERGISE_LOCKING_PIN:
            if (digitalRead(STROBE_SIGNAL_STATUS_PIN) == LOW && millis() - time_since_last_state_transition > STROBE_SIGNAL_MARGIN_MS) {
                if (millis() - time_since_last_state_transition <= ENGAGE_LOCKING_PIN_MAX_LAG) {
                    digitalWrite(INDEXING_SOLENOID_PIN, HIGH);
                    next_instruction = WAIT_FOR_INDEX_CONTROL_SWITCH_AND_STOP_MOTOR;
                    time_since_last_state_transition = millis();
                } else {
                    stop_and_disengage_everything();
                    next_instruction = ERROR_STATE;
                    time_since_last_state_transition = millis();
                }
            }


        // Wait for the index control switch to engage, then stop the motor
        case WAIT_FOR_INDEX_CONTROL_SWITCH_AND_STOP_MOTOR:
            if (digitalRead(INDEXING_CONTROL_SWITCH_PIN) == HIGH) {
                digitalWrite(TURRET_CLOCKWISE_RUN_PIN, LOW);
                next_instruction = WAIT_THEN_REVERSE_MOTOR;
                time_since_last_state_transition = millis();
            }


        // Wait for TIME_BETWEEN_MOTOR_DIRECTION_REVERSAL milliseconds, then start reversing the motor
        case WAIT_THEN_REVERSE_MOTOR:
            if (millis() - time_since_last_state_transition >= TIME_BETWEEN_MOTOR_DIRECTION_REVERSAL) {
                digitalWrite(TURRET_COUNTER_CLOCKWISE_RUN_PIN, HIGH);
                next_instruction = WAIT_FOR_LOCKING_CONTROL_SWITCH_AND_STOP_MOTOR;
                time_since_last_state_transition = millis();
            }


        // Once the locking pin hits home, stop the motor.
        case WAIT_FOR_LOCKING_CONTROL_SWITCH_AND_STOP_MOTOR:
            if (digitalRead(LOCKING_CONTROL_SWITCH_PIN) == HIGH) {
                if (millis() - time_since_last_state_transition <= REACT_LOCKING_CONTROL_SWITH_MAX_LAG) {
                    digitalWrite(TURRET_COUNTER_CLOCKWISE_RUN_PIN, LOW);
                    next_instruction = WAIT_THEN_DISENGAGE_LOCKING_PIN;
                    time_since_last_state_transition = millis();
                } else {
                    stop_and_disengage_everything();
                    next_instruction = ERROR_STATE;
                    time_since_last_state_transition = millis();
                }
            }

        
        // After 200ms, release the locking pin.
        case WAIT_THEN_DISENGAGE_LOCKING_PIN:
            if (millis() - time_since_last_state_transition >= TIME_TO_DISENGAGE_LOCKING_PIN) {
                digitalWrite(INDEXING_SOLENOID_PIN, LOW);
                next_instruction = END_RUN_CYCLE_RESET_STATE;
                time_since_last_state_transition = millis();
           }


        // Cycle complete, reset all pins to a known value.
        // This performs the same operation regardless of a successful run or an error
        case END_RUN_CYCLE_RESET_STATE:
        case ERROR_STATE:
            stop_and_disengage_everything();
            next_instruction = READY_TO_START_RUN_CYCLE;
            time_since_last_state_transition = millis();

        // This is just here in case 
        default: 
            break;
    }
}
