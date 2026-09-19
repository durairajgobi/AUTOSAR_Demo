/**
 * @file DoorControl_App.c
 * @brief AUTOSAR Classic Door Control Application - Sample Implementation
 * 
 * This file implements the Door Control Software Component with the following features:
 * - Door open/close control logic
 * - Position feedback monitoring
 * - Motor current monitoring (overcurrent detection)
 * - Timeout detection (jam detection)
 * - Diagnostic support (DIDs, Routines, DTCs)
 * 
 * @version 1.0
 * @date 2024
 */

#include "DoorControl.h"
#include "Rte_DoorControlSWC.h"
#include "Rte_DiagnosticsSWC.h"
#include "Com.h"
#include "Dem.h"
#include "Dcm.h"

/* =============================================================================
 * CONSTANTS AND DEFINITIONS
 * ============================================================================= */

#define DOOR_CLOSED_POSITION        0
#define DOOR_OPEN_POSITION          100
#define DOOR_POSITION_TOLERANCE     5
#define MOTOR_MAX_CURRENT_MA        5000
#define MOTOR_OVERCURRENT_LIMIT_MA  4500
#define DOOR_MOVEMENT_TIMEOUT_MS    200
#define MOTOR_SPEED_MAX             100
#define MOTOR_SPEED_MIN             -100

/* Door State Machine States */
typedef enum {
    DOOR_STATE_IDLE,
    DOOR_STATE_OPENING,
    DOOR_STATE_CLOSING,
    DOOR_STATE_OPEN,
    DOOR_STATE_CLOSED,
    DOOR_STATE_ERROR,
    DOOR_STATE_JAMMED
} DoorStateType;

/* Door Error Codes */
typedef enum {
    DOOR_ERROR_NONE = 0,
    DOOR_ERROR_OVERCURRENT = 1,
    DOOR_ERROR_TIMEOUT = 2,
    DOOR_ERROR_SENSOR = 3,
    DOOR_ERROR_MOTOR = 4,
    DOOR_ERROR_CAN = 5,
    DOOR_ERROR_FAULT_MEMORY = 6
} DoorErrorCodeType;

/* =============================================================================
 * GLOBAL VARIABLES
 * ============================================================================= */

/* Door Control State Machine */
static DoorStateType g_DoorState = DOOR_STATE_IDLE;
static DoorStateType g_DoorPreviousState = DOOR_STATE_IDLE;

/* Door Position and Feedback */
static uint16 g_DoorPosition = 0;      /* Current position 0-100% */
static uint16 g_DoorTargetPosition = 0; /* Target position */
static uint16 g_MotorCurrent_mA = 0;   /* Motor current in mA */
static uint8 g_DoorError = DOOR_ERROR_NONE;

/* Timing Variables */
static uint32 g_MovementTimeCounter_ms = 0;
static uint32 g_CommandTimeoutCounter_ms = 0;

/* Diagnostic Error Memory */
static DoorErrorCodeType g_ErrorHistory[10];
static uint8 g_ErrorHistoryIndex = 0;

/* Configuration */
static uint8 g_MotorSpeed = 0;
static sint8 g_MotorDirection = 0;

/* =============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================= */

/**
 * @brief Read position sensor value
 * @return Position value 0-100%
 */
static uint16 DoorControl_ReadPositionSensor(void) {
    uint16 adcValue;
    uint16 position;
    
    /* Read ADC value from sensor (0-10000 raw counts -> 0-100%) */
    adcValue = Adc_ReadChannel(ADC_CHANNEL_POSITION);
    
    /* Convert to percentage */
    position = (adcValue * 100) / 10000;
    
    /* Validate range */
    if (position > 100) {
        position = 100;
    }
    
    return position;
}

/**
 * @brief Read motor current sensor
 * @return Current in milliamps
 */
static uint16 DoorControl_ReadMotorCurrent(void) {
    uint16 adcValue;
    uint16 current_mA;
    
    /* Read ADC value from current sensor */
    adcValue = Adc_ReadChannel(ADC_CHANNEL_CURRENT);
    
    /* Convert to milliamps (0-5000 mA range) */
    current_mA = (adcValue * 5000) / 10000;
    
    return current_mA;
}

/**
 * @brief Set motor speed and direction
 * @param[in] speed Motor speed: -100 to +100 (-100=full close, +100=full open)
 */
static void DoorControl_SetMotorSpeed(sint8 speed) {
    /* Limit speed range */
    if (speed > MOTOR_SPEED_MAX) {
        speed = MOTOR_SPEED_MAX;
    }
    if (speed < MOTOR_SPEED_MIN) {
        speed = MOTOR_SPEED_MIN;
    }
    
    if (speed > 0) {
        /* Open direction */
        Dio_WriteChannel(DIO_CHANNEL_MOTOR_DIR, 1);
        Pwm_SetDutyCycle(PWM_CHANNEL_MOTOR, (uint8)speed);
    } else if (speed < 0) {
        /* Close direction */
        Dio_WriteChannel(DIO_CHANNEL_MOTOR_DIR, 0);
        Pwm_SetDutyCycle(PWM_CHANNEL_MOTOR, (uint8)(-speed));
    } else {
        /* Stop */
        Pwm_SetDutyCycle(PWM_CHANNEL_MOTOR, 0);
    }
    
    g_MotorSpeed = (uint8)((speed < 0) ? -speed : speed);
    g_MotorDirection = (speed == 0) ? 0 : (speed > 0 ? 1 : -1);
}

/**
 * @brief Handle overcurrent condition
 */
static void DoorControl_HandleOvercurrent(void) {
    /* Stop motor immediately */
    DoorControl_SetMotorSpeed(0);
    
    /* Set error state */
    g_DoorState = DOOR_STATE_ERROR;
    g_DoorError = DOOR_ERROR_OVERCURRENT;
    
    /* Report DTC */
    Dem_SetEventStatus(DEM_EVENT_OVERCURRENT, DEM_EVENT_STATUS_FAILED);
    
    /* Store in error history */
    g_ErrorHistory[g_ErrorHistoryIndex++] = DOOR_ERROR_OVERCURRENT;
    if (g_ErrorHistoryIndex >= 10) {
        g_ErrorHistoryIndex = 0;
    }
}

/**
 * @brief Handle door jam/timeout
 */
static void DoorControl_HandleJam(void) {
    /* Stop motor */
    DoorControl_SetMotorSpeed(0);
    
    /* Set jam state */
    g_DoorState = DOOR_STATE_JAMMED;
    g_DoorError = DOOR_ERROR_TIMEOUT;
    
    /* Report DTC */
    Dem_SetEventStatus(DEM_EVENT_TIMEOUT, DEM_EVENT_STATUS_FAILED);
    
    /* Store in error history */
    g_ErrorHistory[g_ErrorHistoryIndex++] = DOOR_ERROR_TIMEOUT;
    if (g_ErrorHistoryIndex >= 10) {
        g_ErrorHistoryIndex = 0;
    }
}

/**
 * @brief Update door state machine
 */
static void DoorControl_UpdateStateMachine(void) {
    g_DoorPreviousState = g_DoorState;
    
    switch (g_DoorState) {
        case DOOR_STATE_IDLE:
            if (g_DoorTargetPosition > (g_DoorPosition + DOOR_POSITION_TOLERANCE)) {
                /* Start opening */
                g_DoorState = DOOR_STATE_OPENING;
                g_MovementTimeCounter_ms = 0;
                DoorControl_SetMotorSpeed(80); /* 80% speed */
            } else if (g_DoorTargetPosition < (g_DoorPosition - DOOR_POSITION_TOLERANCE)) {
                /* Start closing */
                g_DoorState = DOOR_STATE_CLOSING;
                g_MovementTimeCounter_ms = 0;
                DoorControl_SetMotorSpeed(-80); /* -80% speed */
            }
            break;
            
        case DOOR_STATE_OPENING:
            g_MovementTimeCounter_ms += 10; /* Assuming 10ms cycle */
            
            /* Check for timeout */
            if (g_MovementTimeCounter_ms > DOOR_MOVEMENT_TIMEOUT_MS) {
                DoorControl_HandleJam();
                break;
            }
            
            /* Check if reached target */
            if (g_DoorPosition >= (DOOR_OPEN_POSITION - DOOR_POSITION_TOLERANCE)) {
                g_DoorState = DOOR_STATE_OPEN;
                DoorControl_SetMotorSpeed(0);
            }
            break;
            
        case DOOR_STATE_CLOSING:
            g_MovementTimeCounter_ms += 10;
            
            /* Check for timeout */
            if (g_MovementTimeCounter_ms > DOOR_MOVEMENT_TIMEOUT_MS) {
                DoorControl_HandleJam();
                break;
            }
            
            /* Check if reached target */
            if (g_DoorPosition <= (DOOR_CLOSED_POSITION + DOOR_POSITION_TOLERANCE)) {
                g_DoorState = DOOR_STATE_CLOSED;
                DoorControl_SetMotorSpeed(0);
            }
            break;
            
        case DOOR_STATE_OPEN:
        case DOOR_STATE_CLOSED:
            /* Hold position, only move if command changes */
            if (g_DoorTargetPosition != g_DoorPosition) {
                g_DoorState = DOOR_STATE_IDLE;
            }
            break;
            
        case DOOR_STATE_ERROR:
        case DOOR_STATE_JAMMED:
            /* Stay in error state until cleared via diagnostics */
            DoorControl_SetMotorSpeed(0);
            break;
            
        default:
            g_DoorState = DOOR_STATE_IDLE;
            break;
    }
}

/**
 * @brief Check motor current for faults
 */
static void DoorControl_CheckMotorCurrent(void) {
    if (g_MotorSpeed > 0) {  /* Motor is running */
        if (g_MotorCurrent_mA > MOTOR_OVERCURRENT_LIMIT_MA) {
            DoorControl_HandleOvercurrent();
        }
    }
}

/* =============================================================================
 * RUNNABLE ENTITIES (Called by RTE)
 * ============================================================================= */

/**
 * @brief DoorControl Runnable - Main control logic (10ms cycle)
 * 
 * This runnable implements:
 * - State machine execution
 * - Sensor reading and validation
 * - Fault detection
 * - Output command generation
 */
void DoorControl_Runnable_10ms(void) {
    uint16 position;
    uint16 current;
    uint8 doorCommand = 0;
    Std_ReturnType status;
    
    /* ========== INPUT ACQUISITION ========== */
    
    /* Read door command from CAN (0x100, position 0-1) */
    status = Rte_Read_DoorCommand_In_DoorOpenCmd(&doorCommand);
    if (status != RTE_E_OK) {
        g_DoorError = DOOR_ERROR_CAN;
    }
    
    if (doorCommand == 1) {
        g_DoorTargetPosition = DOOR_OPEN_POSITION;
    } else {
        /* Default to closed */
        g_DoorTargetPosition = DOOR_CLOSED_POSITION;
    }
    
    /* ========== SENSOR READING ========== */
    
    /* Read position sensor */
    position = DoorControl_ReadPositionSensor();
    g_DoorPosition = position;
    
    /* Read motor current */
    current = DoorControl_ReadMotorCurrent();
    g_MotorCurrent_mA = current;
    
    /* ========== FAULT DETECTION ========== */
    
    /* Check overcurrent */
    DoorControl_CheckMotorCurrent();
    
    /* ========== STATE MACHINE ========== */
    
    DoorControl_UpdateStateMachine();
    
    /* ========== OUTPUT TRANSMISSION ========== */
    
    /* Write door status to CAN (0x101) */
    Rte_Write_DoorStatus_Out_DoorPosition(g_DoorPosition);
    Rte_Write_DoorStatus_Out_DoorStatus((uint8)g_DoorState);
    Rte_Write_DoorStatus_Out_DoorError(g_DoorError);
    
    /* Write motor current */
    Rte_ISignal_Send_DoorMotorCurrent(g_MotorCurrent_mA);
    
    /* Trigger CAN transmission */
    Com_IpduTriggerTransmit(IPDU_DOOR_STATUS);
}

/**
 * @brief Diagnostics Runnable - Diagnostic data handling (50ms cycle)
 * 
 * This runnable:
 * - Updates DID values
 * - Handles diagnostic requests
 * - Monitors system health
 */
void Diagnostics_Runnable_50ms(void) {
    uint8 diagData[8];
    Std_ReturnType status;
    
    /* Update DID values in memory for diagnostic reads */
    
    /* DID 0xF190: Door Position */
    g_DiagData.DoorPosition = g_DoorPosition;
    
    /* DID 0xF191: Motor Current */
    g_DiagData.MotorCurrent = g_MotorCurrent_mA;
    
    /* DID 0xF192: Door State */
    g_DiagData.DoorState = (uint8)g_DoorState;
    
    /* DID 0xF193: Error Code */
    g_DiagData.ErrorCode = g_DoorError;
    
    /* Check system health for passive DTC reporting */
    if (g_DoorState == DOOR_STATE_ERROR) {
        Dem_SetEventStatus(DEM_EVENT_GENERAL_ERROR, DEM_EVENT_STATUS_FAILED);
    } else {
        Dem_SetEventStatus(DEM_EVENT_GENERAL_ERROR, DEM_EVENT_STATUS_PASSED);
    }
}

/* =============================================================================
 * DIAGNOSTIC CALLBACKS
 * ============================================================================= */

/**
 * @brief Read DID callback - Called by DCM on 0x22 (ReadDataByIdentifier)
 */
Std_ReturnType Diag_ReadDID_Callback(uint16 did, uint8 *dataBuffer, uint16 *dataLength) {
    switch (did) {
        case 0xF190:  /* Door Position */
            dataBuffer[0] = (uint8)(g_DoorPosition >> 8);
            dataBuffer[1] = (uint8)(g_DoorPosition & 0xFF);
            *dataLength = 2;
            return RTE_E_OK;
            
        case 0xF191:  /* Motor Current */
            dataBuffer[0] = (uint8)(g_MotorCurrent_mA >> 8);
            dataBuffer[1] = (uint8)(g_MotorCurrent_mA & 0xFF);
            *dataLength = 2;
            return RTE_E_OK;
            
        case 0xF192:  /* Door State */
            dataBuffer[0] = (uint8)g_DoorState;
            *dataLength = 1;
            return RTE_E_OK;
            
        case 0xF193:  /* Error Code */
            dataBuffer[0] = g_DoorError;
            *dataLength = 1;
            return RTE_E_OK;
            
        case 0xF180:  /* Hardware Version */
            dataBuffer[0] = 0x00;
            dataBuffer[1] = 0x00;
            dataBuffer[2] = 0x00;
            dataBuffer[3] = 0x01;
            *dataLength = 4;
            return RTE_E_OK;
            
        case 0xF181:  /* Software Version */
            dataBuffer[0] = 0x01;
            dataBuffer[1] = 0x00;
            dataBuffer[2] = 0x00;
            dataBuffer[3] = 0x02;
            *dataLength = 4;
            return RTE_E_OK;
            
        default:
            return RTE_E_INVALID_DID;
    }
}

/**
 * @brief Routine Control callback - Called by DCM on 0x31 (RoutineControl)
 */
Std_ReturnType Diag_RoutineControl_Callback(uint8 subFunction, uint16 routineId, 
                                            uint8 *optionRecord, uint8 *responseBuffer) {
    switch (routineId) {
        case 0x0001:  /* Door Self Test */
            if (subFunction == 0x01) {  /* Start */
                /* Test motor: move to 50% and back */
                g_DoorTargetPosition = 50;
                responseBuffer[0] = 0x00;  /* Test started */
                return RTE_E_OK;
            } else if (subFunction == 0x03) {  /* Request results */
                if ((g_DoorState == DOOR_STATE_IDLE) && (g_DoorError == DOOR_ERROR_NONE)) {
                    responseBuffer[0] = 0x00;  /* PASS */
                    responseBuffer[1] = 0x00;  /* No error */
                } else {
                    responseBuffer[0] = 0x01;  /* FAIL */
                    responseBuffer[1] = g_DoorError;
                }
                return RTE_E_OK;
            }
            break;
            
        case 0x0003:  /* Calibrate Sensor */
            if (subFunction == 0x01) {  /* Start calibration */
                /* Move to fully open, then fully closed */
                g_DoorTargetPosition = 100;
                responseBuffer[0] = 0x00;  /* Calibration started */
                return RTE_E_OK;
            }
            break;
            
        default:
            return RTE_E_INVALID_ROUTINE;
    }
    
    return RTE_E_OK;
}

/**
 * @brief Clear Diagnostics Memory callback - Called by DCM on 0x14
 */
Std_ReturnType Diag_ClearMemory_Callback(void) {
    /* Clear error codes */
    g_DoorError = DOOR_ERROR_NONE;
    
    /* Clear error history */
    uint8 i;
    for (i = 0; i < 10; i++) {
        g_ErrorHistory[i] = 0;
    }
    g_ErrorHistoryIndex = 0;
    
    /* Clear DEM fault memory */
    Dem_ClearEventMemory();
    
    return RTE_E_OK;
}

/**
 * @brief Enable/Disable Door Control (via NM or diagnostic command)
 */
void DoorControl_Enable(uint8 enable) {
    if (!enable) {
        /* Emergency stop */
        DoorControl_SetMotorSpeed(0);
        g_DoorTargetPosition = g_DoorPosition;  /* Hold position */
    }
}

/* =============================================================================
 * INITIALIZATION
 * ============================================================================= */

/**
 * @brief Initialize Door Control Component
 */
void DoorControl_Init(void) {
    /* Initialize hardware */
    Adc_Init();
    Pwm_Init();
    Dio_Init();
    
    /* Initialize state */
    g_DoorState = DOOR_STATE_IDLE;
    g_DoorPosition = 0;
    g_DoorTargetPosition = 0;
    g_MotorCurrent_mA = 0;
    g_DoorError = DOOR_ERROR_NONE;
    
    /* Initialize diagnostic subsystem */
    Dem_Init();
    Dcm_Init();
    
    /* Motor stop */
    DoorControl_SetMotorSpeed(0);
}

/* =============================================================================
 * END OF FILE
 * ============================================================================= */
