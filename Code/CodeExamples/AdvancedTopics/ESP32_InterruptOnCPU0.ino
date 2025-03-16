// Timer handle
hw_timer_t* myTimer = NULL;
// Interrupt flag
volatile bool toggleFlag = false;
unsigned int counter = 0;

// Timer Interrupt Service Routine (ISR) on CPU0
void IRAM_ATTR onTimer() {
    counter++;
}

// Function to configure the timer and link ISR to CPU0
void setupTimerOnCPU0() {
    // Initialize Timer (Timer 0, Divider = 80 → 1 tick = 1µs)
    myTimer = timerBegin(0, 80, true);

    // Set alarm to trigger ISR every 1000 µs (1 ms)
    timerAttachInterrupt(myTimer, &onTimer, true);
    timerAlarmWrite(myTimer, 1000, true);
    timerAlarmEnable(myTimer);
}

void setup() {
    Serial.begin(115200);
    pinMode(10, OUTPUT); // Set GPIO10 as output

    // Create task pinned to CPU0
    xTaskCreatePinnedToCore(
        [](void* pvParameters) {
            setupTimerOnCPU0();
            vTaskDelete(NULL); // Self-delete after setting up timer
        },
        "TimerSetupTask",  // Task name
        2048,              // Stack size
        NULL,              // Task parameters
        1,                 // Priority
        NULL,              // Task handle
        0                  // Run on CPU0
    );
}

void loop() 
{
    printf("Counter = %d\n", counter);
    delay(1000);
}