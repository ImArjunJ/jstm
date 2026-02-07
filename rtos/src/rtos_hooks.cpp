#include "FreeRTOS.h"
#include "task.h"

extern "C" {

extern void xPortSysTickHandler(void);

void vApplicationMallocFailedHook() {
  taskDISABLE_INTERRUPTS();
  for (;;) {
  }
}

void vApplicationStackOverflowHook(TaskHandle_t, char*) {
  taskDISABLE_INTERRUPTS();
  for (;;) {
  }
}

static StaticTask_t idle_task_tcb;
static StackType_t idle_task_stack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(
    StaticTask_t** ppxIdleTaskTCBBuffer, StackType_t** ppxIdleTaskStackBuffer,
    configSTACK_DEPTH_TYPE* pulIdleTaskStackSize) {
  *ppxIdleTaskTCBBuffer = &idle_task_tcb;
  *ppxIdleTaskStackBuffer = idle_task_stack;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t timer_task_tcb;
static StackType_t timer_task_stack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(
    StaticTask_t** ppxTimerTaskTCBBuffer, StackType_t** ppxTimerTaskStackBuffer,
    configSTACK_DEPTH_TYPE* pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &timer_task_tcb;
  *ppxTimerTaskStackBuffer = timer_task_stack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void SysTick_Handler(void) {
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    xPortSysTickHandler();
  }
}
}
