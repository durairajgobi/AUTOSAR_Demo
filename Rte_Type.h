/**
 * @file Rte_Type.h
 * @brief RTE Type Definitions
 * 
 * Defines all data types used across RTE and SWCs.
 * AUTOSAR 4.3.1 Compliant
 * 
 * @version 1.0
 * @date 2024
 */

#ifndef RTE_TYPE_H
#define RTE_TYPE_H

#include <stdint.h>
#include <stdbool.h>

/* =============================================================================
 * STANDARD TYPES (AUTOSAR)
 * ============================================================================= */

typedef uint8_t uint8;
typedef int8_t sint8;
typedef uint16_t uint16;
typedef int16_t sint16;
typedef uint32_t uint32;
typedef int32_t sint32;
typedef uint64_t uint64;
typedef float float32;
typedef double float64;

typedef uint8_t boolean;
#define TRUE  1
#define FALSE 0

typedef uint8_t Std_ReturnType;
#define E_OK    0x00
#define E_NOT_OK 0x01

/* =============================================================================
 * DOOR CONTROL TYPES
 * ============================================================================= */

/* Door State Enumeration */
typedef enum {
    DOOR_STATE_IDLE = 0,
    DOOR_STATE_OPENING = 1,
    DOOR_STATE_CLOSING = 2,
    DOOR_STATE_OPEN = 3,
    DOOR_STATE_CLOSED = 4,
    DOOR_STATE_ERROR = 5,
    DOOR_STATE_JAMMED = 6
} DoorStateType;

/* Door Error Code Enumeration */
typedef enum {
    DOOR_ERROR_NONE = 0,
    DOOR_ERROR_OVERCURRENT = 1,
    DOOR_ERROR_TIMEOUT = 2,
    DOOR_ERROR_SENSOR = 3,
    DOOR_ERROR_MOTOR = 4,
    DOOR_ERROR_CAN = 5,
    DOOR_ERROR_FAULT_MEMORY = 6
} DoorErrorCodeType;

/* Motor Direction Enumeration */
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_OPEN = 1,
    MOTOR_CLOSE = -1
} MotorDirectionType;

/* Door Status Structure */
typedef struct {
    uint16 position;        /* Door position 0-100% */
    uint16 motorCurrent;    /* Motor current in mA */
    DoorStateType state;    /* Door state */
    DoorErrorCodeType error; /* Error code */
} DoorStatusType;

/* Door Command Structure */
typedef struct {
    boolean openCmd;        /* Open command flag */
    boolean closeCmd;       /* Close command flag */
} DoorCommandType;

/* Sensor Data Structure */
typedef struct {
    uint16 position;        /* Position sensor value */
    uint16 current;         /* Current sensor value */
    boolean valid;          /* Data validity flag */
} SensorDataType;

/* Diagnostic Data Structure */
typedef struct {
    uint32 hardwareVersion;
    uint32 softwareVersion;
    uint16 doorPosition;
    uint16 motorCurrent;
    uint8 doorState;
    uint8 errorCode;
} DiagnosticDataType;

/* =============================================================================
 * CAN MESSAGE TYPES
 * ============================================================================= */

/* CAN Frame Structure */
typedef struct {
    uint16 canId;           /* CAN identifier */
    uint8 dlc;              /* Data length code (0-8) */
    uint8 data[8];          /* CAN data bytes */
} CanFrameType;

/* CAN Message Status */
typedef enum {
    CAN_MSG_NOT_SENT = 0,
    CAN_MSG_SENT = 1,
    CAN_MSG_RECEIVED = 2,
    CAN_MSG_TIMEOUT = 3,
    CAN_MSG_ERROR = 4
} CanMessageStatusType;

/* =============================================================================
 * DIAGNOSTIC TYPES
 * ============================================================================= */

/* UDS Service IDs */
typedef uint8_t UdsServiceIdType;
#define UDS_SID_READ_DID        0x22
#define UDS_SID_ROUTINE_CONTROL 0x31
#define UDS_SID_READ_DTC        0x19
#define UDS_SID_CLEAR_DTC       0x14
#define UDS_SID_TESTER_PRESENT  0x3E

/* DTC Type */
typedef uint32 DtcType;
#define DTC_OVERCURRENT         0xC10001
#define DTC_TIMEOUT             0xC10002
#define DTC_SENSOR_ERROR        0xC10003
#define DTC_MOTOR_ERROR         0xC10004
#define DTC_JAM_DETECTED        0xC10005

/* DID Type */
typedef uint16 DidType;
#define DID_HARDWARE_VERSION    0xF180
#define DID_SOFTWARE_VERSION    0xF181
#define DID_DOOR_POSITION       0xF190
#define DID_MOTOR_CURRENT       0xF191
#define DID_DOOR_STATE          0xF192
#define DID_DOOR_ERROR          0xF193

/* Diagnostic Request/Response */
typedef struct {
    uint8 sid;              /* Service ID */
    uint8 subFunction;      /* Sub-function (if applicable) */
    uint8 data[256];        /* Request/response data */
    uint16 length;          /* Data length */
} UdsMessageType;

/* =============================================================================
 * PORT/INTERFACE TYPES
 * ============================================================================= */

/* Sender-Receiver Port Type */
typedef struct {
    boolean isUpdated;      /* Data update flag */
    uint32 lastUpdateTime;  /* Last update timestamp */
    void *data;             /* Pointer to port data */
} SrPortType;

/* Client-Server Port Type */
typedef struct {
    boolean isInvoked;      /* Service invocation flag */
    Std_ReturnType result;  /* Service result */
    void *requestData;      /* Request data pointer */
    void *responseData;     /* Response data pointer */
} CsPortType;

/* =============================================================================
 * TIMING TYPES
 * ============================================================================= */

/* Timing Event */
typedef struct {
    uint32 period;          /* Period in milliseconds */
    uint32 lastTime;        /* Last execution time */
    boolean enabled;        /* Enable flag */
} TimingEventType;

/* =============================================================================
 * END OF FILE
 * ============================================================================= */

#endif /* RTE_TYPE_H */
