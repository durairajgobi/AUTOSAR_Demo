/**
 * @file Diagnostics.h
 * @brief Diagnostics Module - UDS (ISO 14229-1) Support Header
 * 
 * Provides interfaces for diagnostic services including:
 * - DID (Data Identifier) reading
 * - Routine control execution
 * - DTC (Diagnostic Trouble Code) reporting
 * - Fault memory management
 * 
 * @version 1.0
 * @date 2024
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdint.h>
#include <stdbool.h>

/* =============================================================================
 * UDS SERVICE DEFINITIONS
 * ============================================================================= */

/* UDS Service IDs */
#define UDS_SERVICE_DIAG_SESSION        0x10
#define UDS_SERVICE_ECU_RESET           0x11
#define UDS_SERVICE_READ_DID            0x22
#define UDS_SERVICE_READ_MEMORY         0x23
#define UDS_SERVICE_ROUTINE_CONTROL     0x31
#define UDS_SERVICE_REQUEST_DOWNLOAD    0x34
#define UDS_SERVICE_REQUEST_UPLOAD      0x35
#define UDS_SERVICE_TRANSFER_DATA       0x36
#define UDS_SERVICE_READ_DTC            0x19
#define UDS_SERVICE_CLEAR_DTC           0x14
#define UDS_SERVICE_READ_EXTENDED_DATA  0x24
#define UDS_SERVICE_READ_SNAPSHOT_DATA  0x24
#define UDS_SERVICE_SECURITY_ACCESS     0x27
#define UDS_SERVICE_TESTER_PRESENT      0x3E
#define UDS_SERVICE_CONTROL_IO          0x2F
#define UDS_SERVICE_WRITE_DID           0x2E

/* UDS Response SID = Request SID + 0x40 */
#define UDS_RESPONSE_OFFSET             0x40

/* UDS Error Codes (Negative Response) */
#define UDS_ERROR_GENERAL_REJECT        0x31
#define UDS_ERROR_SERVICE_NOT_SUPPORTED 0x12
#define UDS_ERROR_SUB_FUNCTION_NOT_SUPPORT 0x12
#define UDS_ERROR_INVALID_MSG_LENGTH    0x13
#define UDS_ERROR_RESPONSE_TOO_LONG     0x14
#define UDS_ERROR_CONDITION_NOT_CORRECT 0x22
#define UDS_ERROR_SECURITY_ACCESS_DENIED 0x33
#define UDS_ERROR_INVALID_KEY           0x35
#define UDS_ERROR_ROUTINE_CONTROL_NOT_ACTIVE 0x32

/* =============================================================================
 * DID DEFINITIONS (0x22: ReadDataByIdentifier)
 * ============================================================================= */

#define DID_HARDWARE_VERSION            0xF180  /**< 4 bytes */
#define DID_SOFTWARE_VERSION            0xF181  /**< 4 bytes */
#define DID_DOOR_POSITION               0xF190  /**< 2 bytes, 0-100% */
#define DID_MOTOR_CURRENT               0xF191  /**< 2 bytes, 0-5000 mA */
#define DID_DOOR_STATE                  0xF192  /**< 1 byte, state enum */
#define DID_DOOR_ERROR                  0xF193  /**< 1 byte, error code */

#define DID_COUNT                       6       /**< Total number of DIDs */

/**
 * @brief DID Structure
 */
typedef struct {
    uint16 identifier;
    uint8 length;
    uint8 *data;
} DidType;

/**
 * @brief DID Data Buffer
 */
typedef struct {
    uint32 hardwareVersion;
    uint32 softwareVersion;
    uint16 doorPosition;
    uint16 motorCurrent;
    uint8 doorState;
    uint8 doorError;
} DidDataType;

/* =============================================================================
 * ROUTINE CONTROL DEFINITIONS (0x31: RoutineControl)
 * ============================================================================= */

/* Routine IDs */
#define ROUTINE_ID_SELF_TEST            0x0001
#define ROUTINE_ID_FULL_CYCLE           0x0002
#define ROUTINE_ID_CALIBRATE_SENSOR     0x0003

/* Sub-Functions for Routine Control */
#define ROUTINE_SUB_START               0x01  /**< Start routine */
#define ROUTINE_SUB_STOP                0x02  /**< Stop routine */
#define ROUTINE_SUB_REQUEST_RESULT      0x03  /**< Request results */

/* Routine Status */
typedef enum {
    ROUTINE_STATUS_IDLE = 0,
    ROUTINE_STATUS_RUNNING = 1,
    ROUTINE_STATUS_PASSED = 2,
    ROUTINE_STATUS_FAILED = 3,
    ROUTINE_STATUS_NOT_SUPPORTED = 4
} RoutineStatusType;

/**
 * @brief Routine Structure
 */
typedef struct {
    uint16 routineId;
    RoutineStatusType status;
    uint32 startTime;
    uint32 expectedDuration;
    uint8 resultCode;
    uint8 resultLength;
    uint8 resultData[8];
} RoutineType;

/* =============================================================================
 * DTC DEFINITIONS (0x19: ReadDTCInformation)
 * ============================================================================= */

/* DTC IDs */
#define DTC_OVERCURRENT_ERROR           0xC10001
#define DTC_TIMEOUT_ERROR               0xC10002
#define DTC_SENSOR_ERROR                0xC10003
#define DTC_MOTOR_ERROR                 0xC10004
#define DTC_JAM_DETECTED                0xC10005

#define DTC_COUNT                       5

/* DTC Status Masks */
#define DTC_STATUS_TEST_FAILED          0x01
#define DTC_STATUS_TEST_NOT_COMPLETED   0x02
#define DTC_STATUS_CONFIRMED            0x04
#define DTC_STATUS_TEST_NOT_COMPLETED_THIS_CYCLE 0x08
#define DTC_STATUS_WARNING_INDICATOR_PENDING 0x10
#define DTC_STATUS_WARNING_INDICATOR_OFF 0x20
#define DTC_STATUS_ADVISORY_WARNING_PENDING 0x40
#define DTC_STATUS_ADVISORY_WARNING_OFF 0x80

/* DTC Severity */
typedef enum {
    DTC_SEVERITY_INFO = 0,
    DTC_SEVERITY_WARNING = 1,
    DTC_SEVERITY_ERROR = 2,
    DTC_SEVERITY_CRITICAL = 3
} DtcSeverityType;

/**
 * @brief DTC Structure
 */
typedef struct {
    uint32 dtcCode;
    uint8 status;
    DtcSeverityType severity;
    uint32 occurrenceCounter;
    uint32 firstOccurrenceTime;
    uint32 lastOccurrenceTime;
} DtcType;

/* =============================================================================
 * SESSION CONTROL (0x10: DiagnosticSessionControl)
 * ============================================================================= */

typedef enum {
    DIAG_SESSION_DEFAULT = 0x01,
    DIAG_SESSION_PROGRAMMING = 0x10,
    DIAG_SESSION_EXTENDED = 0x03,
    DIAG_SESSION_SAFETY_SYSTEM = 0x04
} DiagnosticSessionType;

/* =============================================================================
 * SECURITY ACCESS (0x27: SecurityAccess)
 * ============================================================================= */

#define SECURITY_ACCESS_LEVEL           0x05  /**< Security access level */
#define SECURITY_KEY_LENGTH             4     /**< Security key length (bytes) */

/* =============================================================================
 * PUBLIC FUNCTION PROTOTYPES
 * ============================================================================= */

/**
 * @brief Initialize Diagnostics Module
 * 
 * @return E_OK if successful
 */
uint8_t Diag_Init(void);

/**
 * @brief Main Diagnostics Task
 * 
 * Processes diagnostic requests and updates DTC status.
 * Called periodically (typically 50ms).
 */
void Diag_Task(void);

/**
 * @brief Process UDS Request
 * 
 * @param[in] request: Pointer to UDS request data
 * @param[in] requestLength: Request data length
 * @param[out] response: Pointer to response buffer
 * @param[out] responseLength: Pointer to response length
 * @return E_OK if processed, error code otherwise
 */
uint8_t Diag_ProcessRequest(uint8_t *request, uint16_t requestLength,
                             uint8_t *response, uint16_t *responseLength);

/* =============================================================================
 * DID FUNCTIONS (Service 0x22)
 * ============================================================================= */

/**
 * @brief Read Data by Identifier
 * 
 * @param[in] did: Data Identifier
 * @param[out] data: Pointer to data buffer
 * @param[out] length: Length of returned data
 * @return E_OK if DID found
 */
uint8_t Diag_ReadDID(uint16_t did, uint8_t *data, uint16_t *length);

/**
 * @brief Write Data Identifier
 * 
 * @param[in] did: Data Identifier
 * @param[in] data: Pointer to data to write
 * @param[in] length: Length of data
 * @return E_OK if successful
 */
uint8_t Diag_WriteDID(uint16_t did, uint8_t *data, uint16_t length);

/**
 * @brief Update DID Value
 * 
 * @param[in] did: Data Identifier
 * @param[in] value: New value (uint32)
 * @return E_OK if successful
 */
uint8_t Diag_UpdateDID(uint16_t did, uint32_t value);

/**
 * @brief Get DID Data
 * 
 * @param[out] didData: Pointer to DID data structure
 * @return E_OK if successful
 */
uint8_t Diag_GetDIDData(DidDataType *didData);

/* =============================================================================
 * ROUTINE CONTROL FUNCTIONS (Service 0x31)
 * ============================================================================= */

/**
 * @brief Start Routine
 * 
 * @param[in] routineId: Routine identifier
 * @param[in] optionRecord: Option record (if any)
 * @return E_OK if routine started
 */
uint8_t Diag_StartRoutine(uint16_t routineId, uint8_t *optionRecord);

/**
 * @brief Stop Routine
 * 
 * @param[in] routineId: Routine identifier
 * @return E_OK if routine stopped
 */
uint8_t Diag_StopRoutine(uint16_t routineId);

/**
 * @brief Request Routine Results
 * 
 * @param[in] routineId: Routine identifier
 * @param[out] resultBuffer: Buffer for routine results
 * @param[out] resultLength: Length of results
 * @return E_OK if results available
 */
uint8_t Diag_RequestRoutineResults(uint16_t routineId, 
                                    uint8_t *resultBuffer, 
                                    uint16_t *resultLength);

/**
 * @brief Get Routine Status
 * 
 * @param[in] routineId: Routine identifier
 * @return Routine status
 */
RoutineStatusType Diag_GetRoutineStatus(uint16_t routineId);

/* =============================================================================
 * DTC FUNCTIONS (Service 0x19)
 * ============================================================================= */

/**
 * @brief Read Diagnostic Trouble Codes
 * 
 * @param[out] dtcBuffer: Buffer for DTC data
 * @param[out] dtcCount: Number of DTCs returned
 * @return E_OK if successful
 */
uint8_t Diag_ReadDTC(uint8_t *dtcBuffer, uint16_t *dtcCount);

/**
 * @brief Read Number of DTCs
 * 
 * @return Number of active DTCs
 */
uint16_t Diag_ReadDTCCount(void);

/**
 * @brief Check if DTC is Active
 * 
 * @param[in] dtcCode: DTC code
 * @return true if DTC is active, false otherwise
 */
bool Diag_IsDTCActive(uint32_t dtcCode);

/**
 * @brief Set DTC
 * 
 * @param[in] dtcCode: DTC code
 * @param[in] severity: DTC severity
 * @return E_OK if set
 */
uint8_t Diag_SetDTC(uint32_t dtcCode, DtcSeverityType severity);

/**
 * @brief Clear DTC
 * 
 * @param[in] dtcCode: DTC code (0 to clear all)
 * @return E_OK if cleared
 */
uint8_t Diag_ClearDTC(uint32_t dtcCode);

/**
 * @brief Get DTC Information
 * 
 * @param[in] dtcCode: DTC code
 * @param[out] dtc: Pointer to DTC structure
 * @return E_OK if DTC found
 */
uint8_t Diag_GetDTCInfo(uint32_t dtcCode, DtcType *dtc);

/* =============================================================================
 * SESSION FUNCTIONS (Service 0x10)
 * ============================================================================= */

/**
 * @brief Set Diagnostic Session
 * 
 * @param[in] session: Diagnostic session type
 * @return E_OK if successful
 */
uint8_t Diag_SetSession(DiagnosticSessionType session);

/**
 * @brief Get Current Session
 * 
 * @return Current diagnostic session
 */
DiagnosticSessionType Diag_GetCurrentSession(void);

/* =============================================================================
 * SECURITY FUNCTIONS (Service 0x27)
 * ============================================================================= */

/**
 * @brief Request Security Access
 * 
 * @param[in] level: Security level
 * @param[out] seedBuffer: Buffer for seed data
 * @param[out] seedLength: Length of seed
 * @return E_OK if seed provided
 */
uint8_t Diag_RequestSecurityAccess(uint8_t level, 
                                    uint8_t *seedBuffer, 
                                    uint16_t *seedLength);

/**
 * @brief Send Security Key
 * 
 * @param[in] level: Security level
 * @param[in] keyBuffer: Buffer with key data
 * @param[in] keyLength: Length of key
 * @return E_OK if key accepted
 */
uint8_t Diag_SendSecurityKey(uint8_t level, 
                              uint8_t *keyBuffer, 
                              uint16_t keyLength);

/**
 * @brief Check if Security Access Granted
 * 
 * @param[in] level: Security level
 * @return true if access granted
 */
bool Diag_IsSecurityAccessGranted(uint8_t level);

/* =============================================================================
 * CLEAR DIAGNOSTICS FUNCTIONS (Service 0x14)
 * ============================================================================= */

/**
 * @brief Clear All Diagnostic Information
 * 
 * Clears all DTCs, fault memory, error counters, and diagnostic data.
 * 
 * @return E_OK if successful
 */
uint8_t Diag_ClearAllDiagnostics(void);

/**
 * @brief Clear Fault Memory
 * 
 * @return E_OK if successful
 */
uint8_t Diag_ClearFaultMemory(void);

/* =============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================= */

/**
 * @brief Check if Diagnostic Service is Enabled
 * 
 * @param[in] serviceId: UDS service ID
 * @return true if enabled
 */
bool Diag_IsServiceEnabled(uint8_t serviceId);

/**
 * @brief Enable/Disable Diagnostic Service
 * 
 * @param[in] serviceId: UDS service ID
 * @param[in] enabled: true=enable, false=disable
 */
void Diag_EnableService(uint8_t serviceId, bool enabled);

/**
 * @brief Get Diagnostic Statistics
 * 
 * @param[out] activeDTCs: Number of active DTCs
 * @param[out] historicDTCs: Number of historic DTCs
 * @param[out] sessionTime: Time in current session (ms)
 */
void Diag_GetStatistics(uint16_t *activeDTCs, uint16_t *historicDTCs, 
                        uint32_t *sessionTime);

/**
 * @brief Log Diagnostic Event
 * 
 * @param[in] eventCode: Event code
 * @param[in] eventData: Event-specific data
 */
void Diag_LogEvent(uint8_t eventCode, uint32_t eventData);

/* =============================================================================
 * ERROR CODES
 * ============================================================================= */

#define DIAG_E_OK                   0x00
#define DIAG_E_INVALID_DID          0x31  /**< DID not supported */
#define DIAG_E_INVALID_ROUTINE      0x32  /**< Routine not supported */
#define DIAG_E_ROUTINE_RUNNING      0x33  /**< Routine already running */
#define DIAG_E_SERVICE_NOT_SUPPORT  0x12  /**< Service not supported */
#define DIAG_E_INVALID_LENGTH       0x13  /**< Invalid message length */
#define DIAG_E_RESPONSE_TOO_LONG    0x14  /**< Response too long */

#endif /* DIAGNOSTICS_H */
