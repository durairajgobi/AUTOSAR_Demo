/**
 * @file DoorControl.h
 * @brief Door Control System - Main Header File
 * 
 * This header defines all public interfaces, types, and constants
 * for the Door Control System (DoorControlSWC and DiagnosticsSWC).
 * 
 * @version 1.0
 * @date 2024
 */

#ifndef DOOR_CONTROL_H
#define DOOR_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

/* =============================================================================
 * TYPE DEFINITIONS
 * ============================================================================= */

/**
 * @brief Door State Type
 */
typedef enum {
    DOOR_STATE_IDLE = 0,      /**< No movement, position stable */
    DOOR_STATE_OPENING = 1,   /**< Door opening in progress */
    DOOR_STATE_CLOSING = 2,   /**< Door closing in progress */
    DOOR_STATE_OPEN = 3,      /**< Door fully open */
    DOOR_STATE_CLOSED = 4,    /**< Door fully closed */
    DOOR_STATE_ERROR = 5,     /**< Error condition */
    DOOR_STATE_JAMMED = 6     /**< Door jam detected */
} DoorStateType;

/**
 * @brief Door Error Codes
 */
typedef enum {
    DOOR_ERROR_NONE = 0,           /**< No error */
    DOOR_ERROR_OVERCURRENT = 1,    /**< Motor overcurrent detected */
    DOOR_ERROR_TIMEOUT = 2,        /**< Movement timeout (jam) */
    DOOR_ERROR_SENSOR = 3,         /**< Position sensor error */
    DOOR_ERROR_MOTOR = 4,          /**< Motor driver error */
    DOOR_ERROR_CAN = 5,            /**< CAN communication error */
    DOOR_ERROR_FAULT_MEMORY = 6    /**< Fault memory error */
} DoorErrorCodeType;

/**
 * @brief Motor Direction
 */
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_OPEN = 1,
    MOTOR_CLOSE = -1
} MotorDirectionType;

/**
 * @brief Return Type (AUTOSAR Standard)
 */
typedef uint8_t Std_ReturnType;
#define E_OK    0x00
#define E_NOT_OK 0x01

/* =============================================================================
 * CONSTANTS
 * ============================================================================= */

/* Door Position Constants */
#define DOOR_CLOSED_POSITION        0      /**< Door fully closed (0%) */
#define DOOR_OPEN_POSITION          100    /**< Door fully open (100%) */
#define DOOR_POSITION_TOLERANCE     5      /**< Position tolerance (±%) */

/* Motor Control Constants */
#define MOTOR_MAX_SPEED             100    /**< Maximum motor speed (%) */
#define MOTOR_MIN_SPEED             -100   /**< Minimum motor speed (%) */
#define MOTOR_OVERCURRENT_LIMIT_MA  4500   /**< Overcurrent limit (mA) */
#define MOTOR_NORMAL_CURRENT_MA     2000   /**< Normal operating current (mA) */

/* Timing Constants */
#define DOOR_MOVEMENT_TIMEOUT_MS    200    /**< Jam detection timeout (ms) */
#define DOOR_CONTROL_CYCLE_MS       10     /**< Door control cycle (ms) */
#define DIAG_CYCLE_MS               50     /**< Diagnostic cycle (ms) */

/* CAN Message IDs */
#define CAN_ID_DOOR_COMMAND         0x100  /**< Door command message */
#define CAN_ID_DOOR_STATUS          0x101  /**< Door status message */
#define CAN_ID_DIAGNOSTICS          0x7D0  /**< Diagnostics message (UDS) */

/* DID Identifiers */
#define DID_HARDWARE_VERSION        0xF180 /**< Hardware version */
#define DID_SOFTWARE_VERSION        0xF181 /**< Software version */
#define DID_DOOR_POSITION           0xF190 /**< Door position */
#define DID_MOTOR_CURRENT           0xF191 /**< Motor current */
#define DID_DOOR_STATE              0xF192 /**< Door state */
#define DID_DOOR_ERROR              0xF193 /**< Error code */

/* Routine Identifiers */
#define ROUTINE_SELF_TEST           0x0001 /**< Door self-test routine */
#define ROUTINE_FULL_CYCLE          0x0002 /**< Full open/close cycle */
#define ROUTINE_CALIBRATE_SENSOR    0x0003 /**< Sensor calibration routine */

/* DTC Identifiers */
#define DTC_OVERCURRENT             0xC10001 /**< Motor overcurrent DTC */
#define DTC_TIMEOUT                 0xC10002 /**< Movement timeout DTC */
#define DTC_SENSOR_ERROR            0xC10003 /**< Sensor error DTC */
#define DTC_MOTOR_ERROR             0xC10004 /**< Motor error DTC */
#define DTC_JAM_DETECTED            0xC10005 /**< Jam detected DTC */

/* =============================================================================
 * DATA STRUCTURES
 * ============================================================================= */

/**
 * @brief Door Status Structure
 */
typedef struct {
    uint16 position;           /**< Current position (0-100%) */
    uint16 motorCurrent_mA;    /**< Motor current (mA) */
    DoorStateType state;       /**< Current state */
    DoorErrorCodeType error;   /**< Error code */
} DoorStatusType;

/**
 * @brief Door Command Structure
 */
typedef struct {
    uint8 openCmd;             /**< Open command (0/1) */
    uint8 closeCmd;            /**< Close command (0/1) */
} DoorCommandType;

/**
 * @brief Diagnostic Data Structure
 */
typedef struct {
    uint32 hardwareVersion;    /**< Hardware version */
    uint32 softwareVersion;    /**< Software version */
    uint16 doorPosition;       /**< Current door position */
    uint16 motorCurrent;       /**< Current motor current */
    uint8 doorState;           /**< Current door state */
    uint8 errorCode;           /**< Current error code */
} DiagnosticDataType;

/**
 * @brief Sensor Data Structure
 */
typedef struct {
    uint16 position;           /**< Raw position sensor value */
    uint16 current;            /**< Raw current sensor value */
    bool valid;                /**< Data validity flag */
} SensorDataType;

/* =============================================================================
 * GLOBAL VARIABLES
 * ============================================================================= */

extern DoorStatusType g_DoorStatus;
extern DiagnosticDataType g_DiagData;
extern uint8 g_DoorError;
extern DoorStateType g_DoorState;

/* =============================================================================
 * PUBLIC FUNCTION PROTOTYPES
 * ============================================================================= */

/**
 * @brief Initialize Door Control System
 * 
 * Initializes all hardware, software, and diagnostic modules.
 * Must be called once during system startup.
 * 
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType DoorControl_Init(void);

/**
 * @brief De-initialize Door Control System
 * 
 * Shuts down all components and stops motor.
 * 
 * @return E_OK if successful
 */
Std_ReturnType DoorControl_Deinit(void);

/**
 * @brief Main Door Control Runnable (10ms cycle)
 * 
 * Executes the door control state machine, reads sensors,
 * detects faults, and sends status via CAN.
 * 
 * Called by RTE every 10ms.
 */
void DoorControl_Runnable_10ms(void);

/**
 * @brief Diagnostic Runnable (50ms cycle)
 * 
 * Updates diagnostic data, handles DID requests,
 * and reports system health.
 * 
 * Called by RTE every 50ms.
 */
void Diagnostics_Runnable_50ms(void);

/**
 * @brief Enable/Disable Door Control
 * 
 * @param[in] enable: true=enable, false=disable (emergency stop)
 */
void DoorControl_Enable(bool enable);

/**
 * @brief Set Door Target Position
 * 
 * @param[in] position: Target position (0-100%)
 * @return E_OK if valid, E_NOT_OK if out of range
 */
Std_ReturnType DoorControl_SetTargetPosition(uint8 position);

/**
 * @brief Get Current Door Status
 * 
 * @param[out] status: Pointer to status structure
 * @return E_OK if successful
 */
Std_ReturnType DoorControl_GetStatus(DoorStatusType *status);

/**
 * @brief Clear Door Error and Fault Memory
 * 
 * @return E_OK if successful
 */
Std_ReturnType DoorControl_ClearError(void);

/**
 * @brief Read Position Sensor
 * 
 * @return Position value (0-100%)
 */
uint16 DoorControl_ReadPosition(void);

/**
 * @brief Read Motor Current
 * 
 * @return Current in milliamps
 */
uint16 DoorControl_ReadMotorCurrent(void);

/* =============================================================================
 * DIAGNOSTIC FUNCTION PROTOTYPES
 * ============================================================================= */

/**
 * @brief DID Read Callback
 * 
 * Called by DCM when service 0x22 (ReadDataByIdentifier) is received.
 * 
 * @param[in] did: Data Identifier
 * @param[out] dataBuffer: Buffer for DID data
 * @param[out] dataLength: Length of returned data
 * @return E_OK if DID found, error code otherwise
 */
Std_ReturnType Diag_ReadDID_Callback(uint16 did, uint8 *dataBuffer, uint16 *dataLength);

/**
 * @brief Routine Control Callback
 * 
 * Called by DCM when service 0x31 (RoutineControl) is received.
 * 
 * @param[in] subFunction: Sub-function (0x01=Start, 0x03=GetResults)
 * @param[in] routineId: Routine identifier
 * @param[in] optionRecord: Option record data
 * @param[out] responseBuffer: Response data buffer
 * @return E_OK if successful
 */
Std_ReturnType Diag_RoutineControl_Callback(uint8 subFunction, uint16 routineId, 
                                            uint8 *optionRecord, uint8 *responseBuffer);

/**
 * @brief Clear Diagnostics Memory Callback
 * 
 * Called by DCM when service 0x14 (ClearDiagnosticInformation) is received.
 * 
 * @return E_OK if successful
 */
Std_ReturnType Diag_ClearMemory_Callback(void);

/**
 * @brief Get Diagnostic Data
 * 
 * @param[out] diagData: Pointer to diagnostic data structure
 * @return E_OK if successful
 */
Std_ReturnType Diag_GetData(DiagnosticDataType *diagData);

/* =============================================================================
 * HARDWARE ABSTRACTION LAYER - PROTOTYPES
 * ============================================================================= */

/**
 * @brief Initialize ADC for sensor readings
 */
void Adc_Init(void);

/**
 * @brief Read ADC channel
 * 
 * @param[in] channel: ADC channel number
 * @return ADC value (0-10000 for 0-10V range)
 */
uint16 Adc_ReadChannel(uint8 channel);

/**
 * @brief Initialize PWM for motor control
 */
void Pwm_Init(void);

/**
 * @brief Set PWM duty cycle
 * 
 * @param[in] channel: PWM channel
 * @param[in] dutyCycle: Duty cycle (0-100%)
 */
void Pwm_SetDutyCycle(uint8 channel, uint8 dutyCycle);

/**
 * @brief Initialize DIO for GPIO control
 */
void Dio_Init(void);

/**
 * @brief Write to DIO channel
 * 
 * @param[in] channel: DIO channel
 * @param[in] value: 0 or 1
 */
void Dio_WriteChannel(uint8 channel, uint8 value);

/**
 * @brief Read from DIO channel
 * 
 * @param[in] channel: DIO channel
 * @return 0 or 1
 */
uint8 Dio_ReadChannel(uint8 channel);

/* =============================================================================
 * CHANNEL DEFINITIONS (Hardware specific)
 * ============================================================================= */

#define ADC_CHANNEL_POSITION        0    /**< ADC channel for position sensor */
#define ADC_CHANNEL_CURRENT         1    /**< ADC channel for current sensor */
#define PWM_CHANNEL_MOTOR           0    /**< PWM channel for motor speed */
#define DIO_CHANNEL_MOTOR_DIR       0    /**< DIO channel for motor direction */
#define DIO_CHANNEL_MOTOR_ENABLE    1    /**< DIO channel for motor enable */

/* =============================================================================
 * COMMUNICATION LAYER - PROTOTYPES
 * ============================================================================= */

/**
 * @brief Send CAN message
 * 
 * @param[in] canId: CAN message ID
 * @param[in] data: Pointer to data buffer
 * @param[in] length: Data length (0-8 bytes)
 * @return E_OK if sent, E_NOT_OK if failed
 */
Std_ReturnType Can_Send(uint16 canId, uint8 *data, uint8 length);

/**
 * @brief Receive CAN message
 * 
 * @param[in] canId: CAN message ID
 * @param[out] data: Pointer to data buffer
 * @param[out] length: Actual data length
 * @return E_OK if message received, E_NOT_OK otherwise
 */
Std_ReturnType Can_Receive(uint16 canId, uint8 *data, uint8 *length);

/**
 * @brief Trigger PDU transmission
 * 
 * @param[in] pduId: PDU identifier
 */
void Com_IpduTriggerTransmit(uint8 pduId);

/* =============================================================================
 * TIMER UTILITIES
 * ============================================================================= */

/**
 * @brief Get system time in milliseconds
 * 
 * @return Time in ms (32-bit, wraps around)
 */
uint32 GetTimestampMs(void);

/**
 * @brief Get elapsed time
 * 
 * @param[in] startTime: Start time from GetTimestampMs()
 * @return Elapsed time in ms
 */
uint32 GetElapsedTimeMs(uint32 startTime);

/**
 * @brief Sleep/Delay in milliseconds
 * 
 * @param[in] delayMs: Delay duration in ms
 */
void DelayMs(uint32 delayMs);

#endif /* DOOR_CONTROL_H */
