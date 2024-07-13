/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include"time.h"

#define CCM_RAM __attribute__((section(".ccmram")))

#define QUEUE_SIZE_1 3
#define QUEUE_SIZE_2 10
#define NUMBER_OF_SENDERS 3
//configure High and Low priority

#define HIGH_PRIORITY (configMAX_PRIORITIES - 1)
#define LOW_PRIORITY (configMAX_PRIORITIES - 2)

//Prototypes for each function used in this project

void SenderTask(void *pvParameters);
void ReceiverTask(void *pvParameters);
void SenderTimerCallbackFunction(TimerHandle_t xTimer);
void ReceiverTimerCallbackFunction(TimerHandle_t xTimer);
void ResetSystem();
int createRandomNumbers(int lower_bound,int upper_bound);
void resetCheckArrays();

//Variable declaration of RTOS queues,semaphores and timers

QueueHandle_t xQueue;
SemaphoreHandle_t xSenderSemaphore[NUMBER_OF_SENDERS];
SemaphoreHandle_t xReceiverSemaphore;
TimerHandle_t xSenderTimer[NUMBER_OF_SENDERS];
TimerHandle_t xReceiverTimer;

/* Global variable declaration and initialization of sent,blocked and received messages by
each task(senders and receiver)
*/

int sentMessages[NUMBER_OF_SENDERS] = {0};
int blockedMessages[NUMBER_OF_SENDERS] = {0};
int receivedMessages = 0;
int sum_of_periods[3]={0};
int total_sent_messages[3]={0};
int total_succesful_sent_messages=0;
int total_blocked_messages=0;
TickType_t tick_count;
//Bounds of array to generate random Tsender for sender Tasks

int lowerBoundOfTimers[] = {50, 80, 110, 140, 170, 200};
int upperBoundOfTimers[] = {150, 200, 250, 300, 350, 400};
int boundIndex;

/*Main
 * Description:
 * It creates the queue then calls the reset function the first time in order to
 * reset all counters concerning sent and blocked messages and initializs the array index with 0,
 * but also it resets the queue.
 * Returning from the reset function, the main function creates all tasks(3 senders and 1 receiver), their
 * semaphores and their timers, after that it starts all timers and then the scheduler to begin.
 */
void main(void)
{
	srand(time(NULL));
    xQueue = xQueueCreate(QUEUE_SIZE_1,50);
    ResetSystem();
    for (int i = 0; i < NUMBER_OF_SENDERS; i++) {
        xSenderSemaphore[i] = xSemaphoreCreateBinary();
        char taskName[16];
        snprintf(taskName, 16, "Sender%d", i + 1);
        UBaseType_t priority = (i == 2) ? HIGH_PRIORITY : LOW_PRIORITY;
        xTaskCreate(SenderTask, taskName, configMINIMAL_STACK_SIZE, (void *)(intptr_t)i, priority, NULL);
        int random_num=createRandomNumbers(lowerBoundOfTimers[boundIndex],upperBoundOfTimers[boundIndex]);
        xSenderTimer[i] = xTimerCreate("SenderTimer", pdMS_TO_TICKS(random_num), pdFALSE, (void *)(intptr_t)i, SenderTimerCallbackFunction);
    }
    xReceiverSemaphore = xSemaphoreCreateBinary();
    xTaskCreate(ReceiverTask, "Receiver", configMINIMAL_STACK_SIZE, NULL, LOW_PRIORITY, NULL);
    xReceiverTimer = xTimerCreate("ReceiverTimer", pdMS_TO_TICKS(100), pdFALSE, (void *)0, ReceiverTimerCallbackFunction);
    for (int i=0;i<3;i++)
        	xTimerStart(xSenderTimer[i], 0);
    xTimerStart(xReceiverTimer, 0);
    vTaskStartScheduler();

    for (;;) {}
}

/*
 * CreateRandomNumbers function
 * Description:
 * This function generates random numbers within the lower and upper bounds provided as function
 * parameters, it returns then the resultant random number.
 */

int createRandomNumbers(int lower_bound,int upper_bound)
{
	int random_number=rand() % (upper_bound - lower_bound + 1) + lower_bound;
	return random_number;
}

/*
 * SenderTask function
 * Description:
 * The sender task takes the semaphore and then creates a string that has current time, then
 * it check if it can send messages in queue, if there is an empty place it then increments the
 * sent messages counter, if the queue is full, it increments the blocked messages counter.
 * This function controls the three sender tasks, at then end it changes the period of each
 * sender to another random number like mentioned in project document.
 */

void SenderTask(void *pvParameters)
{
    int taskIndex = (intptr_t)pvParameters;
    char message[32];

    for (;;)
    {
        xSemaphoreTake(xSenderSemaphore[taskIndex], portMAX_DELAY);
        tick_count = xTaskGetTickCount();
        snprintf(message, 32, "Time is %lu", (long unsigned int)tick_count);
        if (xQueueSend(xQueue, &message, 0) == pdPASS) {
            sentMessages[taskIndex]++;
        } else {
            blockedMessages[taskIndex]++;
        }

        int rand_num=createRandomNumbers(lowerBoundOfTimers[boundIndex],upperBoundOfTimers[boundIndex]);
        total_sent_messages[taskIndex]++;
        sum_of_periods[taskIndex]+=rand_num;
        rand_num=pdMS_TO_TICKS(rand_num);
        xTimerChangePeriod(xSenderTimer[taskIndex],rand_num,0);
    }
}
/*
 * ReceiverTask function
 * Description:
 * This function creates a string called message to store the received message from the queue,
 * it then checks if there's a message, if there is it increments the received messages counter, if
 * not then it does nothing, after that it reinitializes the timer with a period 100ms.
 */
void ReceiverTask(void *pvParameters)
{
    char message[32];

    for (;;)
    {
    	//Taking semaphore for receiver
        xSemaphoreTake(xReceiverSemaphore, portMAX_DELAY);

        if (xQueueReceive(xQueue, &message, 0) == pdPASS) {
            receivedMessages++;
            //FOR PRINTING RECEIVED MESSAGES
            //printf("%s\n",message);
        }
        xTimerStart(xReceiverTimer, 0);
    }
}

/*SenderTimerCallbackFunction
 * Description:
 * it releases the semaphore of each sender task when timer expires.
 */

void SenderTimerCallbackFunction(TimerHandle_t xTimer)
{
    int taskIndex = (intptr_t)pvTimerGetTimerID(xTimer);
    xSemaphoreGive(xSenderSemaphore[taskIndex]);
}

/*ReceiverTimerCallbackFunction
 * Description:
 * it releases the semaphore of Receiver task when timer expires.
 * it also checks if number of received messages exceeded 1000 or not, if it exceeds it, it goes
 * to reset function
 */

void ReceiverTimerCallbackFunction(TimerHandle_t xTimer)
{
	//Releasing receiver semaphore
	xSemaphoreGive(xReceiverSemaphore);
    if (receivedMessages>=1000) {
                  ResetSystem();
              }

}

/*ResetSystem Function
 * Description:
 * This function is responsible of initializing queue, counters and array index the first time it is called.
 * After that, when the number of received messages is exceeds 1000, it prints the following:
 * -Total successfully sent messages
 * -Total blocked messages
 * -Total sent messages
 * -Total received messages
 * -Sent and blocked messages per sender task
 * -Average period of each sender
 * After printing the statistics, it calls the resetCheckArrays function
 */
void ResetSystem()
{
	static int first_time=0;
	if(first_time==0)
	{
		xQueueReset(xQueue);
		boundIndex=0;
		 for (int i = 0; i < NUMBER_OF_SENDERS; i++) {
		        sentMessages[i] = 0;
		        blockedMessages[i] = 0;
		        total_sent_messages[i]=0;
		        sum_of_periods[i]=0;
		    }
		    receivedMessages = 0;
		    total_succesful_sent_messages=0;
		    first_time++;
	}
	else
	{
	total_succesful_sent_messages=sentMessages[0]+sentMessages[1]+sentMessages[2];
	total_blocked_messages=blockedMessages[0]+blockedMessages[1]+blockedMessages[2];
    printf("------ Statistics ------\n");

    for (int i = 0; i < NUMBER_OF_SENDERS; i++) {
    	 printf("Total sent and blocked messages by Sender %d:\n Sent: %d Blocked: %d and average period: %d\n",i+1,\
    			 sentMessages[i],blockedMessages[i],(sum_of_periods[i]/total_sent_messages[i]));
    }

    printf("Total successfully sent and blocked messages: \n Sent: %d Blocked: %d\n ",\
    		total_succesful_sent_messages,total_blocked_messages);
    printf("Total received messages: %d\n", receivedMessages);
    printf("------------------------\n");

    // Reset counters
    for (int i = 0; i < NUMBER_OF_SENDERS; i++) {
        sentMessages[i] = 0;
        blockedMessages[i] = 0;
        total_sent_messages[i]=0;
        sum_of_periods[i]=0;
    }
    receivedMessages = 0;
    total_succesful_sent_messages=0;
    total_blocked_messages=0;
    xQueueReset(xQueue);
    resetCheckArrays();
	}
}

/*
 * resetCheckArrays function
 * Description:
 * First of all, this function increments array index, and check if it exceeds the size of the array
 * which is 6 in our case, if yes then it destroys timers and prints game over, if not then it
 * changes the sender timer periods to a new random generated number and prints still going and repeats
 * the process until array index reaches 7.
 */

void resetCheckArrays()
{
    boundIndex++;
    if (boundIndex >= sizeof(lowerBoundOfTimers) / sizeof(lowerBoundOfTimers[0])) {
    	for(int i=0;i<3;i++)
    		xTimerDelete(xSenderTimer[i], portMAX_DELAY);
        printf("Game Over\n");
        exit(0);
    }
    else
    {
    	  for(int i=0;i<3;i++)
    	    {
    	    	int rand=createRandomNumbers(lowerBoundOfTimers[boundIndex],upperBoundOfTimers[boundIndex]);
    	    	xTimerChangePeriod(xSenderTimer[i],pdMS_TO_TICKS(rand),portMAX_DELAY);
    	    }
    	printf("Still going\n");
    }
	}

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
