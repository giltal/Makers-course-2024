// Timer handle
hw_timer_t* myTimer = NULL;
// Interrupt flag
volatile bool toggleFlag = false;
unsigned int counter = 0;

// Timer Interrupt Service Routine (ISR) on CPU0
void IRAM_ATTR onTimer() {
    counter++;
}

void setup() {
    Serial.begin(115200);

    // Initialize Timer (Timer 0, Divider = 80 → 1 tick = 1 µs)
    myTimer = timerBegin(0, 80, true);

    // Attach Interrupt (ISR triggers every 1000 µs = 1 ms)
    timerAttachInterrupt(myTimer, &onTimer, true);
    timerAlarmWrite(myTimer, 1000, true);
    timerAlarmEnable(myTimer); // Start Timer
}

void loop() 
{
    printf("Counter = %d\n", counter);
    delay(1000);
}