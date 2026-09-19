/**
 * @file Rte_DoorControlSWC.c
 * @brief RTE Implementation for DoorControlSWC
 * 
 * Implements the Runtime Environment for the Door Control Software Component.
 * Handles port communication, signal routing, and timing events.
 * 
 * AUTOSAR 4.3.1 Compliant
 * 
 * @version 1.0
 * @date 2024
 */

#include "Rte_DoorControlSWC.h"
#include "Com.h"
#include "Dem.h"
#include "Dcm.h"

/* =============================================================================
 * INTERNAL DATA STRUCTURES
 * ============================================================================= */

/* Port data buffers */
static DoorStatusType g_DoorStatus_Out = {0, 0, DOOR_STATE_IDLE, DOOR_ERROR_NONE};
static DoorCommandType g_DoorCommand_In = {FALSE, FALSE};
static SensorDataType g_DoorSensor_In = {0, 0, FALSE};

/* Port timestamps for validity checking */
static uint32 g_DoorCommand_LastUpdate = 0;
static uint32 g_DoorSensor_LastUpdate = 0;
static uint32 g_DoorStatus_LastUpdate = 0;

/* Communication timeouts (ms) */
#define RTE_CAN_TIMEOUT_MS          500
#define RTE_SENSOR_TIMEOUT_MS       200
#define RTE_DATA_VALIDITY_TIMEOUT   1000

/* Timing event state */
static uint32 g_LastRunnableTime = 0;
static uint32 g_RunnableCounter = 0;

/* =============================================================================
 * SENDER-RECEIVER PORT IMPLEMENTATIONS
 * ============================================================================= */

/**
 * @brief Write door status to output port
 */
Std_ReturnType Rte_Write_DoorStatus_Out_DoorStatus(
    const DoorStatusType *status) {
    
    if (status == NULL) {
        return E_NOT_OK;
    }
    
    /* Copy data to port buffer */
    g_DoorStatus_Out = *status;
    g_DoorStatus_LastUpdate = Rte_GetTimestamp();
    
    /* Send via COM layer (CAN signal transmission) */
    /* The COM module will transmit DoorStatus_PDU (0x101) */
    Com_SendSignal(0, (uint8 *)&g_DoorStatus_Out, sizeof(DoorStatusType));
    
    return E_OK;
}

/**
 * @brief Write door position to output port
 */
Std_ReturnType Rte_Write_DoorStatus_Out_Position(uint16 position) {
    g_DoorStatus_Out.position = position;
    g_DoorStatus_LastUpdate = Rte_GetTimestamp();
    
    /* Send via COM layer */
    Com_SendSignal(0, (uint8 *)&position, sizeof(uint16));
    
    return E_OK;
}

/**
 * @brief Write motor current to output port
 */
Std_ReturnType Rte_Write_DoorStatus_Out_MotorCurrent(uint16 current) {
    g_DoorStatus_Out.motorCurrent = current;
    g_DoorStatus_LastUpdate = Rte_GetTimestamp();
    
    /* Send via COM layer */
    Com_SendSignal(1, (uint8 *)&current, sizeof(uint16));
    
    return E_OK;
}

/**
 * @brief Write door state to output port
 */
Std_ReturnType Rte_Write_DoorStatus_Out_State(DoorStateType state) {
    g_DoorStatus_Out.state = state;
    g_DoorStatus_LastUpdate = Rte_GetTimestamp();
    
    /* Send via COM layer */
    Com_SendSignal(2, (uint8 *)&state, sizeof(uint8));
    
    return E_OK;
}

/**
 * @brief Write door error to output port
 */
Std_ReturnType Rte_Write_DoorStatus_Out_Error(DoorErrorCodeType error) {
    g_DoorStatus_Out.error = error;
    g_DoorStatus_LastUpdate = Rte_GetTimestamp();
    
    /* Send via COM layer */
    Com_SendSignal(3, (uint8 *)&error, sizeof(uint8));
    
    return E_OK;
}

/* =============================================================================
 * RECEIVER PORT IMPLEMENTATIONS
 * ============================================================================= */

/**
 * @brief Read door command from input port
 */
Std_ReturnType Rte_Read_DoorCommand_In_DoorCommand(
    DoorCommandType *command) {
    
    if (command == NULL) {
        return E_NOT_OK;
    }
    
    /* Check data timeout */
    if (Rte_GetTimestamp() - g_DoorCommand_LastUpdate > RTE_CAN_TIMEOUT_MS) {
        /* Data timed out, return error */
        return E_NOT_OK;
    }
    
    /* Copy port data to caller */
    *command = g_DoorCommand_In;
    
    return E_OK;
}

/**
 * @brief Read door open command
 */
Std_ReturnType Rte_Read_DoorCommand_In_OpenCmd(boolean *openCmd) {
    if (openCmd == NULL) {
        return E_NOT_OK;
    }
    
    if (Rte_GetTimestamp() - g_DoorCommand_LastUpdate > RTE_CAN_TIMEOUT_MS) {
        return E_NOT_OK;
    }
    
    *openCmd = g_DoorCommand_In.openCmd;
    return E_OK;
}

/**
 * @brief Read door close command
 */
Std_ReturnType Rte_Read_DoorCommand_In_CloseCmd(boolean *closeCmd) {
    if (closeCmd == NULL) {
        return E_NOT_OK;
    }
    
    if (Rte_GetTimestamp() - g_DoorCommand_LastUpdate > RTE_CAN_TIMEOUT_MS) {
        return E_NOT_OK;
    }
    
    *closeCmd = g_DoorCommand_In.closeCmd;
    return E_OK;
}

/* =============================================================================
 * SENSOR DATA PORT IMPLEMENTATIONS
 * ============================================================================= */

/**
 * @brief Read sensor data from input port
 */
Std_ReturnType Rte_Read_DoorSensor_In_SensorData(
    SensorDataType *sensor) {
    
    if (sensor == NULL) {
        return E_NOT_OK;
    }
    
    /* Check data timeout */
    if (Rte_GetTimestamp() - g_DoorSensor_LastUpdate > RTE_SENSOR_TIMEOUT_MS) {
        return E_NOT_OK;
    }
    
    /* Check data validity flag */
    if (!g_DoorSensor_In.valid) {
        return E_NOT_OK;
    }
    
    *sensor = g_DoorSensor_In;
    return E_OK;
}

/**
 * @brief Read position sensor
 */
Std_ReturnType Rte_Read_DoorSensor_In_Position(uint16 *position) {
    if (position == NULL) {
        return E_NOT_OK;
    }
    
    if (Rte_GetTimestamp() - g_DoorSensor_LastUpdate > RTE_SENSOR_TIMEOUT_MS) {
        return E_NOT_OK;
    }
    
    if (!g_DoorSensor_In.valid) {
        return E_NOT_OK;
    }
    
    *position = g_DoorSensor_In.position;
    return E_OK;
}

/**
 * @brief Read current sensor
 */
Std_ReturnType Rte_Read_DoorSensor_In_Current(uint16 *current) {
    if (current == NULL) {
        return E_NOT_OK;
    }
    
    if (Rte_GetTimestamp() - g_DoorSensor_LastUpdate > RTE_SENSOR_TIMEOUT_MS) {
        return E_NOT_OK;
    }
    
    if (!g_DoorSensor_In.valid) {
        return E_NOT_OK;
    }
    
    *current = g_DoorSensor_In.current;
    return E_OK;
}

/* =============================================================================
 * CLIENT-SERVER PORT IMPLEMENTATIONS
 * ============================================================================= */

/**
 * @brief Set motor speed and direction
 */
Std_ReturnType Rte_Call_MotorControl_Out_SetMotorSpeed(
    sint8 speed,
    MotorDirectionType direction) {
    
    /* Validate parameters */
    if (speed < -100 || speed > 100) {
        return E_NOT_OK;
    }
    
    /* Call motor control service */
    /* In real implementation, this would call:
     * - PWM driver to set speed
     * - GPIO driver to set direction
     * - Overcurrent monitoring
     */
    
    /* For simulation, just return OK */
    return E_OK;
}

/**
 * @brief Stop motor
 */
Std_ReturnType Rte_Call_MotorControl_Out_StopMotor(void) {
    /* Stop motor by setting speed to 0 */
    return Rte_Call_MotorControl_Out_SetMotorSpeed(0, MOTOR_STOP);
}

/**
 * @brief Enable/disable motor
 */
Std_ReturnType Rte_Call_MotorControl_Out_EnableMotor(boolean enable) {
    /* Enable/disable motor via GPIO */
    /* In real implementation: set enable GPIO pin */
    return E_OK;
}

/* =============================================================================
 * TIMING EVENT IMPLEMENTATIONS
 * ============================================================================= */

/**
 * @brief Get current system time
 */
uint32 Rte_GetTimestamp(void) {
    /* Return system time in milliseconds */
    /* In real implementation, read from timer/SysTick */
    extern uint32 GetTimestampMs(void);
    return GetTimestampMs();
}

/**
 * @brief Check if timing event is due
 */
boolean Rte_CheckTimingEvent(uint32 lastTime, uint32 period) {
    uint32 currentTime = Rte_GetTimestamp();
    uint32 elapsed = currentTime - lastTime;
    
    return (elapsed >= period) ? TRUE : FALSE;
}

/* =============================================================================
 * RTE INITIALIZATION
 * ============================================================================= */

/**
 * @brief Initialize RTE for DoorControlSWC
 * 
 * Sets up initial values, timestamps, and port states.
 */
void Rte_Init_DoorControlSWC(void) {
    uint32 currentTime = Rte_GetTimestamp();
    
    /* Initialize port data */
    g_DoorStatus_Out.position = 0;
    g_DoorStatus_Out.motorCurrent = 0;
    g_DoorStatus_Out.state = DOOR_STATE_IDLE;
    g_DoorStatus_Out.error = DOOR_ERROR_NONE;
    
    g_DoorCommand_In.openCmd = FALSE;
    g_DoorCommand_In.closeCmd = FALSE;
    
    g_DoorSensor_In.position = 0;
    g_DoorSensor_In.current = 0;
    g_DoorSensor_In.valid = FALSE;
    
    /* Initialize timestamps */
    g_DoorCommand_LastUpdate = currentTime;
    g_DoorSensor_LastUpdate = currentTime;
    g_DoorStatus_LastUpdate = currentTime;
    
    /* Initialize timing */
    g_LastRunnableTime = currentTime;
    g_RunnableCounter = 0;
}

/* =============================================================================
 * DATA RECEPTION (COM -> RTE)
 * ============================================================================= */

/**
 * @brief Receive door command from CAN
 * 
 * Called by COM layer when DoorCommand PDU is received.
 */
void Rte_ReceiveDoorCommand(const uint8 *data, uint8 length) {
    if (data != NULL && length >= 2) {
        /* Parse CAN message: [openCmd, closeCmd, ...] */
        g_DoorCommand_In.openCmd = (data[0] & 0x01) ? TRUE : FALSE;
        g_DoorCommand_In.closeCmd = (data[1] & 0x01) ? TRUE : FALSE;
        g_DoorCommand_LastUpdate = Rte_GetTimestamp();
    }
}

/**
 * @brief Receive sensor data from ADC/HAL
 * 
 * Called by HAL when sensor data is available.
 */
void Rte_ReceiveSensorData(uint16 position, uint16 current, boolean valid) {
    g_DoorSensor_In.position = position;
    g_DoorSensor_In.current = current;
    g_DoorSensor_In.valid = valid;
    g_DoorSensor_LastUpdate = Rte_GetTimestamp();
}

/* =============================================================================
 * ERROR HANDLING
 * ============================================================================= */

/**
 * @brief RTE error handler
 */
void Rte_ErrorHandler(uint8 errorCode) {
    /* Log error */
    Dem_SetEventStatus(errorCode, TRUE);
}

/**
 * @brief Check data validity
 */
boolean Rte_IsDataValid(void) {
    uint32 currentTime = Rte_GetTimestamp();
    
    /* Check if sensor data is valid and fresh */
    if (currentTime - g_DoorSensor_LastUpdate > RTE_DATA_VALIDITY_TIMEOUT) {
        return FALSE;
    }
    
    if (!g_DoorSensor_In.valid) {
        return FALSE;
    }
    
    return TRUE;
}

/* =============================================================================
 * RUNNABLE SCHEDULER
 * ============================================================================= */

/**
 * @brief Schedule and execute 10ms runnable
 */
void Rte_Schedule_10ms(void) {
    uint32 currentTime = Rte_GetTimestamp();
    
    /* Check if 10ms period has elapsed */
    if (currentTime - g_LastRunnableTime >= 10) {
        /* Call the actual runnable */
        DoorControl_Runnable_10ms();
        
        /* Update timing */
        g_LastRunnableTime = currentTime;
        g_RunnableCounter++;
    }
}

/**
 * @brief Get runnable execution count
 */
uint32 Rte_GetRunnableCount(void) {
    return g_RunnableCounter;
}

/* =============================================================================
 * END OF FILE
 * ============================================================================= */
