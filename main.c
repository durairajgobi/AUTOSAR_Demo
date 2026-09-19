/**
 * @file main.c
 * @brief Door Control System - Main Entry Point
 * 
 * System initialization, task scheduling, and main event loop.
 * 
 * @version 1.0
 * @date 2024
 */

#include "DoorControl.h"
#include "Diagnostics.h"

/* =============================================================================
 * SYSTEM CONFIGURATION
 * ============================================================================= */

#define SYSTEM_VERSION_MAJOR    1
#define SYSTEM_VERSION_MINOR    0
#define SYSTEM_VERSION_PATCH    0

#define SYSTEM_TICK_MS          1       /* 1ms system tick */
#define DOOR_CONTROL_CYCLE_MS   10      /* 10ms door control */
#define DIAG_CYCLE_MS           50      /* 50ms diagnostics */

/* =============================================================================
 * GLOBAL TASK COUNTERS
 * ============================================================================= */

static uint32_t g_TickCount = 0;
static uint32_t g_DoorControlCounter = 0;
static uint32_t g_DiagCounter = 0;

/* =============================================================================
 * SYSTEM STATE
 * ============================================================================= */

typedef enum {
    SYSTEM_STATE_INIT = 0,
    SYSTEM_STATE_READY = 1,
    SYSTEM_STATE_RUNNING = 2,
    SYSTEM_STATE_ERROR = 3,
    SYSTEM_STATE_SHUTDOWN = 4
} SystemStateType;

static SystemStateType g_SystemState = SYSTEM_STATE_INIT;
static uint32_t g_SystemUptime = 0;

/* =============================================================================
 * INITIALIZATION FUNCTIONS
 * ============================================================================= */

/**
 * @brief Early initialization (hardware setup)
 * 
 * Called before any other initialization.
 * Sets up basic hardware and system tick.
 */
void System_EarlyInit(void) {
    /* Initialize hardware abstraction layer */
    Hal_Init();
    
    /* Initialize timer for system tick */
    Timer_Init();
    
    g_SystemState = SYSTEM_STATE_READY;
}

/**
 * @brief Application initialization
 * 
 * Called after early init.
 * Sets up application components.
 */
void System_AppInit(void) {
    Std_ReturnType status;
    
    /* Initialize door control system */
    status = DoorControl_Init();
    if (status != E_OK) {
        g_SystemState = SYSTEM_STATE_ERROR;
        return;
    }
    
    /* Initialize diagnostics module */
    status = (Std_ReturnType)Diag_Init();
    if (status != 0) {
        g_SystemState = SYSTEM_STATE_ERROR;
        return;
    }
    
    /* Initialize hardware self-test */
    status = Hal_SelfTest();
    if (status != E_OK) {
        g_SystemState = SYSTEM_STATE_ERROR;
        return;
    }
    
    g_SystemState = SYSTEM_STATE_RUNNING;
}

/**
 * @brief System startup
 * 
 * Main initialization sequence.
 */
void System_Startup(void) {
    /* Early hardware init */
    System_EarlyInit();
    
    if (g_SystemState != SYSTEM_STATE_READY) {
        g_SystemState = SYSTEM_STATE_ERROR;
        return;
    }
    
    /* Application init */
    System_AppInit();
    
    if (g_SystemState != SYSTEM_STATE_RUNNING) {
        g_SystemState = SYSTEM_STATE_ERROR;
        return;
    }
}

/**
 * @brief Shutdown system
 */
void System_Shutdown(void) {
    /* Stop door immediately */
    DoorControl_Enable(false);
    
    /* Clear error state */
    DoorControl_ClearError();
    
    /* Set state */
    g_SystemState = SYSTEM_STATE_SHUTDOWN;
}

/* =============================================================================
 * SYSTEM TICK HANDLER
 * ============================================================================= */

/**
 * @brief System tick (1ms)
 * 
 * Called by timer interrupt every 1ms.
 * Schedules application tasks.
 */
void System_TickHandler(void) {
    g_TickCount++;
    g_SystemUptime++;
    
    /* Schedule 10ms door control task */
    if ((g_TickCount % DOOR_CONTROL_CYCLE_MS) == 0) {
        g_DoorControlCounter++;
    }
    
    /* Schedule 50ms diagnostics task */
    if ((g_TickCount % DIAG_CYCLE_MS) == 0) {
        g_DiagCounter++;
    }
}

/* =============================================================================
 * TASK EXECUTION
 * ============================================================================= */

/**
 * @brief Execute door control task (10ms)
 */
void Task_DoorControl(void) {
    if (g_SystemState != SYSTEM_STATE_RUNNING) {
        return;
    }
    
    DoorControl_Runnable_10ms();
}

/**
 * @brief Execute diagnostics task (50ms)
 */
void Task_Diagnostics(void) {
    if (g_SystemState != SYSTEM_STATE_RUNNING) {
        return;
    }
    
    Diagnostics_Runnable_50ms();
    Diag_Task();
}

/**
 * @brief Execute communication task
 * 
 * Processes CAN messages and diagnostic requests.
 */
void Task_Communication(void) {
    uint8_t canData[8];
    uint8_t canLength;
    uint16_t canId;
    Std_ReturnType status;
    
    if (g_SystemState != SYSTEM_STATE_RUNNING) {
        return;
    }
    
    /* Process CAN messages */
    
    /* Receive door command messages */
    status = Can_Receive(CAN_ID_DOOR_COMMAND, canData, &canLength);
    if (status == E_OK && canLength >= 1) {
        /* Command processed by DoorControl_Runnable */
    }
    
    /* Receive diagnostic requests */
    status = Can_Receive(CAN_ID_DIAGNOSTICS, canData, &canLength);
    if (status == E_OK && canLength >= 1) {
        /* Diagnostic request would be processed here */
    }
}

/* =============================================================================
 * MAIN LOOP
 * ============================================================================= */

/**
 * @brief Main application loop
 * 
 * Executes pending tasks based on tick counter.
 */
void System_MainLoop(void) {
    static uint32_t lastDoorControl = 0;
    static uint32_t lastDiag = 0;
    
    if (g_SystemState != SYSTEM_STATE_RUNNING) {
        return;
    }
    
    /* Execute door control task every 10ms */
    if (g_DoorControlCounter > lastDoorControl) {
        Task_DoorControl();
        lastDoorControl = g_DoorControlCounter;
    }
    
    /* Execute diagnostics task every 50ms */
    if (g_DiagCounter > lastDiag) {
        Task_Diagnostics();
        Task_Communication();
        lastDiag = g_DiagCounter;
    }
}

/* =============================================================================
 * ERROR HANDLING
 * ============================================================================= */

/**
 * @brief Handle system error
 * 
 * @param[in] errorCode: Error code
 */
void System_HandleError(uint8_t errorCode) {
    /* Stop all operations */
    DoorControl_Enable(false);
    
    /* Log error */
    Diag_LogEvent(0x01, errorCode);
    
    /* Set system error state */
    g_SystemState = SYSTEM_STATE_ERROR;
}

/**
 * @brief Get system state
 * 
 * @return Current system state
 */
SystemStateType System_GetState(void) {
    return g_SystemState;
}

/**
 * @brief Get system uptime
 * 
 * @return Uptime in milliseconds
 */
uint32_t System_GetUptime(void) {
    return g_SystemUptime;
}

/* =============================================================================
 * DEBUG/TESTING FUNCTIONS
 * ============================================================================= */

/**
 * @brief Send test CAN message
 * 
 * @param[in] doorOpen: true=open, false=close
 */
void System_SendTestCommand(bool doorOpen) {
    uint8_t data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    
    if (doorOpen) {
        data[0] = 0x01;  /* DoorOpenCmd */
    } else {
        data[1] = 0x01;  /* DoorCloseCmd */
    }
    
    Can_Send(CAN_ID_DOOR_COMMAND, data, 2);
}

/**
 * @brief Get system diagnostics
 * 
 * @param[out] doorStatus: Door status
 * @param[out] dtcCount: Number of active DTCs
 * @param[out] uptime: System uptime (ms)
 */
void System_GetDiagnostics(DoorStatusType *doorStatus, uint16_t *dtcCount, 
                           uint32_t *uptime) {
    if (doorStatus != NULL) {
        DoorControl_GetStatus(doorStatus);
    }
    
    if (dtcCount != NULL) {
        *dtcCount = Diag_ReadDTCCount();
    }
    
    if (uptime != NULL) {
        *uptime = System_GetUptime();
    }
}

/* =============================================================================
 * ENTRY POINT
 * ============================================================================= */

/**
 * @brief Main function
 * 
 * Entry point for the application.
 * 
 * Execution flow:
 * 1. Initialize hardware and software
 * 2. Enter main event loop
 * 3. Execute tasks based on timing
 * 4. Handle errors if needed
 * 5. Shutdown on command
 */
int main(void) {
    /* System startup */
    System_Startup();
    
    /* Check startup status */
    if (g_SystemState != SYSTEM_STATE_RUNNING) {
        System_HandleError(0xFF);  /* Startup error */
        return -1;
    }
    
    /* Main loop (runs forever) */
    while (g_SystemState == SYSTEM_STATE_RUNNING) {
        /* Execute pending tasks */
        System_MainLoop();
        
        /* Optional: Watchdog kick or other background tasks */
    }
    
    /* Graceful shutdown */
    System_Shutdown();
    
    return 0;
}

/* =============================================================================
 * INTERRUPT HANDLERS
 * ============================================================================= */

/**
 * @brief System Tick Interrupt Handler
 * 
 * Called by hardware timer every 1ms.
 * Typically called from ISR context.
 */
void SysTick_Handler(void) {
    System_TickHandler();
    Timer_InterruptHandler();
}

/**
 * @brief CAN Receive Interrupt Handler
 * 
 * Called when CAN message is received.
 */
void CAN_ReceiveHandler(void) {
    Can_ReceiveInterruptHandler();
}

/* =============================================================================
 * END OF FILE
 * ============================================================================= */
