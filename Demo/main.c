#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"

const TickType_t xTimingDelay1 = 200 / portTICK_PERIOD_MS;
const TickType_t xTimingDelay2 = 300 / portTICK_PERIOD_MS;
const TickType_t xTimingDelay3 = 400 / portTICK_PERIOD_MS;

typedef struct TaskInfo_s
{
    int iTaskNumber;
    TickType_t xWCET;
    TickType_t xPeriod;
    TickType_t xRelativeDeadline;
    const char* name;
} TaskInfo_t;


// Define tasks
TaskInfo_t edfLStarDemo[] =
{
    {1, 900, 3000, 2000, "Task 1"},
    {2, 1900, 7000, 5500, "Task 2"},
    {3, 1900, 10000, 6000, "Task 3"}
};

//Overrun Demo
TaskInfo_t overrunDemo[] =
{
    {1, 1000, 2000, 2000, "Task 1"},
    {2, 1000, 4000, 4000, "Task 2"},
};

const int iNumTasks = 2;
TaskInfo_t* tasks = overrunDemo;

BaseType_t wow = pdFALSE;

void TimingTestTask(void *pParam) {
    while(1) {
        TaskInfo_t* xTaskInfo = (TaskInfo_t*) pParam;        
        printk("Started Task iter at: %u\r\n", xTaskGetTickCount());
        printk("Start Timing task %d\r\n", xTaskInfo->iTaskNumber);
        busyWait(xTaskInfo->xWCET);
        if (wow == pdFALSE)
        {
            wow = pdTRUE;
            busyWait(xTaskInfo->xWCET+10000);
            printk("THIS SHOULD NEVER PRINT%d\r\n", xTaskInfo->iTaskNumber);
        }
        printk("Done Timing task %d\r\n", xTaskInfo->iTaskNumber);
        endTaskPeriod();
    }
}

/**
 *	This is the systems main entry, some call it a boot thread.
 *
 *	-- Absolutely nothing wrong with this being called main(), just it doesn't have
 *	-- the same prototype as you'd see in a linux program.
 **/
int main(void) {
    BaseType_t vVal;
    
    rpi_cpu_irq_disable();
    rpi_aux_mu_init();

    vVal = srpInitSRPStacks();
    if (vVal == pdFALSE) {
        printk("Failed to initialize SRP stacks\r\n");
        while (1) {
            
        }
    }
    else {
        printk("Successfully initialized SRP stacks\r\n");
    }
    

    // Create tasksk
    for (int iTaskNum = 0; iTaskNum < iNumTasks; iTaskNum++)
    {
        xTaskCreate(TimingTestTask, tasks[iTaskNum].name, 256, (void *) &tasks[iTaskNum],
                    PRIORITY_EDF, tasks[iTaskNum].xWCET, tasks[iTaskNum].xRelativeDeadline,
                    tasks[iTaskNum].xPeriod, NULL);
    }
    printk("Finish creating tasks\r\n");
    printSchedule();
    verifyEDFExactBound();
    verifyLLBound();
    vTaskStartScheduler();

    /*
     *	We should never get here, but just in case something goes wrong,
     *	we'll place the CPU into a safe loop.
     */
    while(1) {
        ;
    }
}

