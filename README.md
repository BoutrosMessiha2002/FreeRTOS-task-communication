# RTOS Communicating Tasks Project

This repository contains the implementation of an advanced RTOS Communicating Tasks project using FreeRTOS. The project demonstrates task management, inter-task communication, and system optimization through the use of tasks, timers, queues, and semaphores. It is part of the Embedded Systems course at Cairo University, completed in the second year, second term of the Electronics and Electrical Communications Department.

## Project Overview

The objective of this project was to delve deep into Real-Time Operating Systems (RTOS) using FreeRTOS, focusing on managing tasks, timers, queues, and semaphores to handle complex inter-task communication.

## System Components

- **Message Queue**: Central buffer for inter-task communication.
- **Sender Tasks**: Generate and send messages based on priority.
- **Receiver Task**: Processes messages from the queue.
- **Software Timers**: Manage task scheduling and execution.
- **Idle Task**: Optimizes power management and system efficiency.

## System Flow

1. **Initialization Phase**: Initializes necessary hardware and software resources, including peripherals, the FreeRTOS kernel, and memory management features.
2. **Task Creation and Scheduler Start**: Creates tasks using `xTaskCreate`, assigns specific functions, priorities, and stack sizes, and starts the FreeRTOS scheduler.
3. **Task Execution and Inter-task Communication**: Tasks execute based on priority with FreeRTOS preemptive scheduling. Tasks communicate via FreeRTOS mechanisms like queues and semaphores.
4. **Idle Task and Power Management**: The idle task runs when no other tasks are ready, incorporating power-saving features and handling housekeeping functions.

## Implementation Details

1. **FreeRTOS Variables Declaration**:
   - QueueHandle_t xQueue
   - SemaphoreHandle_t xSenderSemaphore[NUMBER_OF_SENDERS]
   - SemaphoreHandle_t xReceiverSemaphore
   - TimerHandle_t xSenderTimer[NUMBER_OF_SENDERS]
   - TimerHandle_t xReceiverTimer

2. **Global Variables Declaration**:
   - Arrays for tracking sent, blocked, and received messages, and periods.

3. **Main Function**:
   - Initializes the queue, resets the system, creates tasks, semaphores, and timers, starts the timers and the scheduler.

4. **Task Functions**:
   - **SenderTask**: Takes semaphore, sends messages to the queue, increments counters, and changes the timer period.
   - **ReceiverTask**: Takes semaphore, receives messages from the queue, and restarts the timer.
   - **SenderTimerCallbackFunction**: Releases the sender semaphore.
   - **ReceiverTimerCallbackFunction**: Releases the receiver semaphore and resets the system if necessary.

5. **ResetSystem Function**:
   - Initializes the queue and counters, prints statistics after a set number of received messages, and resets the system.

## Results and Discussion

- Demonstrated consistent efficiency in task management and message processing.
- Queue size had a noticeable impact on the rate of blocked messages but minimal effect on overall system efficiency.

## Key Takeaways

- Mastery of FreeRTOS and embedded system programming.
- In-depth experience with task scheduling and inter-task communication.
- Enhanced problem-solving skills through real-world application and debugging.

## Getting Started

To get started with this project, clone the repository and follow the instructions provided in the code comments to set up and run the project on your development board.

```bash
git clone https://github.com/BoutrosMessiha2002/RTOS-Communicating-Tasks.git
