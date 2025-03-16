// Queue handle
QueueHandle_t myQueue;

// Sender Task
void SenderTask(void* pvParameters) {
    int count = 0;
    while (1) {
        count++; // Increment counter
        if (xQueueSend(myQueue, &count, portMAX_DELAY) == pdPASS) {
            printf("Sent: %d\n",count);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
    }
}

// Receiver Task
void ReceiverTask(void* pvParameters) {
    int receivedValue;
    while (1) {
        if (xQueueReceive(myQueue, &receivedValue, portMAX_DELAY) == pdTRUE) {
            printf("Received: %d\n",receivedValue);
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Create queue with space for 5 integers
    myQueue = xQueueCreate(5, sizeof(int));

    if (myQueue == NULL) {
        printf("Queue creation failed!\n");
        return;
    }

    // Create sender task
    xTaskCreate(SenderTask, "Sender Task", 2048, NULL, 1, NULL);

    // Create receiver task
    xTaskCreate(ReceiverTask, "Receiver Task", 2048, NULL, 1, NULL);
}

void loop() {
    // Nothing here, tasks handle execution
}