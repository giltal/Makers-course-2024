// Task handles
TaskHandle_t Task1Handle = NULL;
TaskHandle_t Task2Handle = NULL;

// Task 1 function
void Task1(void* pvParameters) {
    while (1) {
        printf("Task 1 Running on core 1\n");
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay 1000ms (1 second)
    }
}

// Task 2 function
void Task2(void* pvParameters) {
    while (1) {
        printf("Task 2 Running on core 0\n");
        vTaskDelay(pdMS_TO_TICKS(500));  // Delay 500ms
    }
}

void setup() {
    Serial.begin(115200);

    // Create Task 1
    xTaskCreatePinnedToCore(
        Task1,            // Function name
        "Task 1",         // Task name
        2048,             // Stack size (in bytes)
        NULL,             // Parameters
        1,                // Priority (higher = more priority)
        &Task1Handle,     // Task handle
        1                // Pin task to core 1
        );

    // Create Task 2
    xTaskCreatePinnedToCore(
        Task2,            // Function name
        "Task 2",         // Task name
        2048,             // Stack size
        NULL,             // Parameters
        1,                // Priority
        &Task2Handle,     // Task handle
        0                // Pin task to core 0
        );
}

void loop() {
    // Empty, since FreeRTOS handles the tasks
}