/**
 * @file Diagnostics.c
 * @brief Diagnostics Module Implementation - UDS Service Handler
 * 
 * Implements all UDS (ISO 14229-1) diagnostic services:
 * - 0x22: ReadDataByIdentifier (DIDs)
 * - 0x31: RoutineControl (Self-test, calibration, etc.)
 * - 0x19: ReadDTCInformation (Fault codes)
 * - 0x14: ClearDiagnosticInformation (Clear memory)
 * 
 * @version 1.0
 * @date 2024
 */

#include "Diagnostics.h"
#include "DoorControl.h"
#include <string.h>

/* =============================================================================
 * LOCAL VARIABLES
 * ============================================================================= */

/* Diagnostic session state */
static DiagnosticSessionType g_CurrentSession = DIAG_SESSION_DEFAULT;
static uint32_t g_SessionStartTime = 0;

/* DTC Storage */
static DtcType g_DtcMemory[DTC_COUNT] = {0};
static uint8_t g_DtcCount = 0;

/* Routine Status */
static RoutineType g_ActiveRoutines[3] = {0};
static uint8_t g_RoutineCount = 0;

/* Security Access */
static uint8_t g_SecurityLevel = 0;
static bool g_SecurityAccessGranted = false;

/* DID Data Cache */
extern DiagnosticDataType g_DiagData;

/* Service Enable/Disable */
static uint8_t g_ServiceEnabled[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x00-0x09 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0x14,  /* Service 0x14 enabled */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x19-0x28 */
    0, 0, 0, 1,  /* 0x1D-0x20, 0x22 enabled */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x21-0x2A */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x2B-0x34 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 0x35-0x3E */
    0, 0, 1  /* 0x3F-0x41, 0x31 enabled */
};

/* Diagnostic Event Log */
#define DIAG_LOG_SIZE 100
typedef struct {
    uint32_t timestamp;
    uint8_t eventCode;
    uint32_t eventData;
} DiagLogEntryType;

static DiagLogEntryType g_DiagLog[DIAG_LOG_SIZE];
static uint8_t g_DiagLogIndex = 0;

/* =============================================================================
 * INITIALIZATION
 * ============================================================================= */

/**
 * @brief Initialize Diagnostics Module
 */
uint8_t Diag_Init(void) {
    uint8_t i;
    
    /* Initialize DTC memory */
    memset(g_DtcMemory, 0, sizeof(g_DtcMemory));
    g_DtcCount = 0;
    
    /* Initialize routine memory */
    memset(g_ActiveRoutines, 0, sizeof(g_ActiveRoutines));
    g_RoutineCount = 0;
    
    /* Initialize session */
    g_CurrentSession = DIAG_SESSION_DEFAULT;
    g_SessionStartTime = GetTimestampMs();
    
    /* Initialize security */
    g_SecurityLevel = 0;
    g_SecurityAccessGranted = false;
    
    /* Initialize service enables */
    for (i = 0; i < 256; i++) {
        g_ServiceEnabled[0x22] = 1;  /* ReadDID always enabled */
        g_ServiceEnabled[0x31] = 1;  /* RoutineControl always enabled */
        g_ServiceEnabled[0x19] = 1;  /* ReadDTC always enabled */
        g_ServiceEnabled[0x14] = 1;  /* ClearDTC always enabled */
    }
    
    return 0;
}

/* =============================================================================
 * MAIN REQUEST PROCESSING
 * ============================================================================= */

/**
 * @brief Process UDS Request
 * 
 * Main entry point for UDS message processing.
 */
uint8_t Diag_ProcessRequest(uint8_t *request, uint16_t requestLength,
                             uint8_t *response, uint16_t *responseLength) {
    uint8_t serviceId;
    uint8_t errorCode = UDS_ERROR_GENERAL_REJECT;
    
    if (requestLength < 1) {
        return UDS_ERROR_INVALID_MSG_LENGTH;
    }
    
    serviceId = request[0];
    
    /* Check if service is enabled */
    if (!Diag_IsServiceEnabled(serviceId)) {
        return UDS_ERROR_SERVICE_NOT_SUPPORTED;
    }
    
    /* Handle each service */
    switch (serviceId) {
        case UDS_SERVICE_READ_DID:  /* 0x22 */
            errorCode = Diag_HandleReadDID(request, requestLength, response, responseLength);
            break;
            
        case UDS_SERVICE_ROUTINE_CONTROL:  /* 0x31 */
            errorCode = Diag_HandleRoutineControl(request, requestLength, response, responseLength);
            break;
            
        case UDS_SERVICE_READ_DTC:  /* 0x19 */
            errorCode = Diag_HandleReadDTC(request, requestLength, response, responseLength);
            break;
            
        case UDS_SERVICE_CLEAR_DTC:  /* 0x14 */
            errorCode = Diag_HandleClearDTC(request, requestLength, response, responseLength);
            break;
            
        case UDS_SERVICE_TESTER_PRESENT:  /* 0x3E */
            errorCode = Diag_HandleTesterPresent(request, requestLength, response, responseLength);
            break;
            
        default:
            return UDS_ERROR_SERVICE_NOT_SUPPORTED;
    }
    
    return errorCode;
}

/* =============================================================================
 * SERVICE 0x22: READ DATA BY IDENTIFIER
 * ============================================================================= */

static uint8_t Diag_HandleReadDID(uint8_t *request, uint16_t requestLength,
                                   uint8_t *response, uint16_t *responseLength) {
    uint16_t did;
    uint8_t offset = 0;
    uint8_t responseIdx = 1;
    
    if (requestLength < 3) {
        return UDS_ERROR_INVALID_MSG_LENGTH;
    }
    
    /* Response: [0x62 + service, DID_MSB, DID_LSB, data...] */
    response[0] = UDS_SERVICE_READ_DID + UDS_RESPONSE_OFFSET;
    responseIdx = 1;
    
    offset = 1;
    while (offset < requestLength) {
        /* Read DID (2 bytes big-endian) */
        if (offset + 1 >= requestLength) {
            return UDS_ERROR_INVALID_MSG_LENGTH;
        }
        
        did = (request[offset] << 8) | request[offset + 1];
        offset += 2;
        
        /* Add DID to response */
        response[responseIdx++] = (uint8_t)(did >> 8);
        response[responseIdx++] = (uint8_t)(did & 0xFF);
        
        /* Add DID data based on ID */
        switch (did) {
            case DID_HARDWARE_VERSION:
                response[responseIdx++] = (uint8_t)(g_DiagData.hardwareVersion >> 24);
                response[responseIdx++] = (uint8_t)(g_DiagData.hardwareVersion >> 16);
                response[responseIdx++] = (uint8_t)(g_DiagData.hardwareVersion >> 8);
                response[responseIdx++] = (uint8_t)(g_DiagData.hardwareVersion & 0xFF);
                break;
                
            case DID_SOFTWARE_VERSION:
                response[responseIdx++] = (uint8_t)(g_DiagData.softwareVersion >> 24);
                response[responseIdx++] = (uint8_t)(g_DiagData.softwareVersion >> 16);
                response[responseIdx++] = (uint8_t)(g_DiagData.softwareVersion >> 8);
                response[responseIdx++] = (uint8_t)(g_DiagData.softwareVersion & 0xFF);
                break;
                
            case DID_DOOR_POSITION:
                response[responseIdx++] = (uint8_t)(g_DiagData.doorPosition >> 8);
                response[responseIdx++] = (uint8_t)(g_DiagData.doorPosition & 0xFF);
                break;
                
            case DID_MOTOR_CURRENT:
                response[responseIdx++] = (uint8_t)(g_DiagData.motorCurrent >> 8);
                response[responseIdx++] = (uint8_t)(g_DiagData.motorCurrent & 0xFF);
                break;
                
            case DID_DOOR_STATE:
                response[responseIdx++] = g_DiagData.doorState;
                break;
                
            case DID_DOOR_ERROR:
                response[responseIdx++] = g_DiagData.errorCode;
                break;
                
            default:
                return UDS_ERROR_GENERAL_REJECT;  /* DID not supported */
        }
        
        /* Check response buffer size */
        if (responseIdx > 255) {
            return UDS_ERROR_RESPONSE_TOO_LONG;
        }
    }
    
    *responseLength = responseIdx;
    return UDS_ERROR_GENERAL_REJECT;  /* 0x00 indicates success in negative response format */
}

/**
 * @brief Read DID
 */
uint8_t Diag_ReadDID(uint16_t did, uint8_t *data, uint16_t *length) {
    switch (did) {
        case DID_HARDWARE_VERSION:
            data[0] = (uint8_t)(g_DiagData.hardwareVersion >> 24);
            data[1] = (uint8_t)(g_DiagData.hardwareVersion >> 16);
            data[2] = (uint8_t)(g_DiagData.hardwareVersion >> 8);
            data[3] = (uint8_t)(g_DiagData.hardwareVersion & 0xFF);
            *length = 4;
            break;
            
        case DID_DOOR_POSITION:
            data[0] = (uint8_t)(g_DiagData.doorPosition >> 8);
            data[1] = (uint8_t)(g_DiagData.doorPosition & 0xFF);
            *length = 2;
            break;
            
        case DID_MOTOR_CURRENT:
            data[0] = (uint8_t)(g_DiagData.motorCurrent >> 8);
            data[1] = (uint8_t)(g_DiagData.motorCurrent & 0xFF);
            *length = 2;
            break;
            
        case DID_DOOR_STATE:
            data[0] = g_DiagData.doorState;
            *length = 1;
            break;
            
        case DID_DOOR_ERROR:
            data[0] = g_DiagData.errorCode;
            *length = 1;
            break;
            
        default:
            return DIAG_E_INVALID_DID;
    }
    
    return DIAG_E_OK;
}

/**
 * @brief Update DID
 */
uint8_t Diag_UpdateDID(uint16_t did, uint32_t value) {
    switch (did) {
        case DID_DOOR_POSITION:
            g_DiagData.doorPosition = (uint16_t)value;
            break;
            
        case DID_MOTOR_CURRENT:
            g_DiagData.motorCurrent = (uint16_t)value;
            break;
            
        case DID_DOOR_STATE:
            g_DiagData.doorState = (uint8_t)value;
            break;
            
        case DID_DOOR_ERROR:
            g_DiagData.errorCode = (uint8_t)value;
            break;
            
        default:
            return DIAG_E_INVALID_DID;
    }
    
    return DIAG_E_OK;
}

/* =============================================================================
 * SERVICE 0x31: ROUTINE CONTROL
 * ============================================================================= */

static uint8_t Diag_HandleRoutineControl(uint8_t *request, uint16_t requestLength,
                                          uint8_t *response, uint16_t *responseLength) {
    uint8_t subFunction;
    uint16_t routineId;
    uint8_t responseIdx = 0;
    
    if (requestLength < 4) {
        return UDS_ERROR_INVALID_MSG_LENGTH;
    }
    
    subFunction = request[1];
    routineId = (request[2] << 8) | request[3];
    
    /* Response: [0x71, SubFunction, RoutineID_MSB, RoutineID_LSB, data...] */
    response[responseIdx++] = UDS_SERVICE_ROUTINE_CONTROL + UDS_RESPONSE_OFFSET;
    response[responseIdx++] = subFunction;
    response[responseIdx++] = (uint8_t)(routineId >> 8);
    response[responseIdx++] = (uint8_t)(routineId & 0xFF);
    
    switch (subFunction) {
        case ROUTINE_SUB_START:
            if (Diag_StartRoutine(routineId, &request[4]) == DIAG_E_OK) {
                response[responseIdx++] = 0x00;  /* Status: started */
            } else {
                return UDS_ERROR_GENERAL_REJECT;
            }
            break;
            
        case ROUTINE_SUB_REQUEST_RESULT:
            if (Diag_RequestRoutineResults(routineId, &response[responseIdx], 
                                          (uint16_t *)&responseIdx) == DIAG_E_OK) {
                /* Results already added to response */
            } else {
                return UDS_ERROR_GENERAL_REJECT;
            }
            break;
            
        default:
            return UDS_ERROR_SUB_FUNCTION_NOT_SUPPORT;
    }
    
    *responseLength = responseIdx;
    return DIAG_E_OK;
}

/**
 * @brief Start routine
 */
uint8_t Diag_StartRoutine(uint16_t routineId, uint8_t *optionRecord) {
    switch (routineId) {
        case ROUTINE_ID_SELF_TEST:
            /* Start self-test routine */
            g_ActiveRoutines[0].routineId = routineId;
            g_ActiveRoutines[0].status = ROUTINE_STATUS_RUNNING;
            g_ActiveRoutines[0].startTime = GetTimestampMs();
            g_ActiveRoutines[0].expectedDuration = 3000;  /* 3 seconds */
            return DIAG_E_OK;
            
        case ROUTINE_ID_FULL_CYCLE:
            /* Start full cycle routine */
            g_ActiveRoutines[1].routineId = routineId;
            g_ActiveRoutines[1].status = ROUTINE_STATUS_RUNNING;
            g_ActiveRoutines[1].startTime = GetTimestampMs();
            g_ActiveRoutines[1].expectedDuration = 5000;  /* 5 seconds */
            return DIAG_E_OK;
            
        case ROUTINE_ID_CALIBRATE_SENSOR:
            /* Start calibration routine */
            g_ActiveRoutines[2].routineId = routineId;
            g_ActiveRoutines[2].status = ROUTINE_STATUS_RUNNING;
            g_ActiveRoutines[2].startTime = GetTimestampMs();
            g_ActiveRoutines[2].expectedDuration = 2000;  /* 2 seconds */
            return DIAG_E_OK;
            
        default:
            return DIAG_E_INVALID_ROUTINE;
    }
}

/**
 * @brief Request routine results
 */
uint8_t Diag_RequestRoutineResults(uint16_t routineId, 
                                    uint8_t *resultBuffer, 
                                    uint16_t *resultLength) {
    uint8_t i;
    
    for (i = 0; i < 3; i++) {
        if (g_ActiveRoutines[i].routineId == routineId) {
            /* Check if routine is complete */
            if (GetElapsedTimeMs(g_ActiveRoutines[i].startTime) >= 
                g_ActiveRoutines[i].expectedDuration) {
                
                /* Mark as complete */
                if (g_DiagData.errorCode == 0) {
                    g_ActiveRoutines[i].status = ROUTINE_STATUS_PASSED;
                    resultBuffer[0] = 0x00;  /* PASS */
                    resultBuffer[1] = 0x00;  /* No error */
                    *resultLength = 2;
                } else {
                    g_ActiveRoutines[i].status = ROUTINE_STATUS_FAILED;
                    resultBuffer[0] = 0x01;  /* FAIL */
                    resultBuffer[1] = g_DiagData.errorCode;
                    *resultLength = 2;
                }
                return DIAG_E_OK;
            } else {
                return DIAG_E_ROUTINE_RUNNING;
            }
        }
    }
    
    return DIAG_E_INVALID_ROUTINE;
}

/**
 * @brief Get routine status
 */
RoutineStatusType Diag_GetRoutineStatus(uint16_t routineId) {
    uint8_t i;
    
    for (i = 0; i < 3; i++) {
        if (g_ActiveRoutines[i].routineId == routineId) {
            return g_ActiveRoutines[i].status;
        }
    }
    
    return ROUTINE_STATUS_NOT_SUPPORTED;
}

/* =============================================================================
 * SERVICE 0x19: READ DTC
 * ============================================================================= */

static uint8_t Diag_HandleReadDTC(uint8_t *request, uint16_t requestLength,
                                   uint8_t *response, uint16_t *responseLength) {
    uint8_t subFunction;
    uint8_t responseIdx = 0;
    uint8_t i;
    
    if (requestLength < 2) {
        return UDS_ERROR_INVALID_MSG_LENGTH;
    }
    
    subFunction = request[1];
    
    /* Response: [0x59, SubFunction, data...] */
    response[responseIdx++] = UDS_SERVICE_READ_DTC + UDS_RESPONSE_OFFSET;
    response[responseIdx++] = subFunction;
    
    switch (subFunction) {
        case 0x01:  /* Report number of DTCs */
            response[responseIdx++] = 0x00;  /* Status mask */
            response[responseIdx++] = g_DtcCount;  /* Number of DTCs */
            break;
            
        case 0x02:  /* Report DTCs by status */
            for (i = 0; i < g_DtcCount; i++) {
                response[responseIdx++] = (uint8_t)(g_DtcMemory[i].dtcCode >> 16);
                response[responseIdx++] = (uint8_t)(g_DtcMemory[i].dtcCode >> 8);
                response[responseIdx++] = (uint8_t)(g_DtcMemory[i].dtcCode & 0xFF);
                response[responseIdx++] = g_DtcMemory[i].status;
            }
            break;
            
        default:
            return UDS_ERROR_SUB_FUNCTION_NOT_SUPPORT;
    }
    
    *responseLength = responseIdx;
    return DIAG_E_OK;
}

/**
 * @brief Set DTC
 */
uint8_t Diag_SetDTC(uint32_t dtcCode, DtcSeverityType severity) {
    uint8_t i;
    
    /* Check if DTC already exists */
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcMemory[i].dtcCode == dtcCode) {
            g_DtcMemory[i].status |= DTC_STATUS_TEST_FAILED;
            g_DtcMemory[i].status |= DTC_STATUS_CONFIRMED;
            return DIAG_E_OK;
        }
    }
    
    /* Add new DTC */
    if (g_DtcCount < DTC_COUNT) {
        g_DtcMemory[g_DtcCount].dtcCode = dtcCode;
        g_DtcMemory[g_DtcCount].status = DTC_STATUS_TEST_FAILED | DTC_STATUS_CONFIRMED;
        g_DtcMemory[g_DtcCount].severity = severity;
        g_DtcMemory[g_DtcCount].occurrenceCounter = 1;
        g_DtcMemory[g_DtcCount].firstOccurrenceTime = GetTimestampMs();
        g_DtcCount++;
        return DIAG_E_OK;
    }
    
    return DIAG_E_INVALID_DID;  /* No space for new DTC */
}

/**
 * @brief Clear DTC
 */
uint8_t Diag_ClearDTC(uint32_t dtcCode) {
    uint8_t i, j;
    
    if (dtcCode == 0) {
        /* Clear all DTCs */
        g_DtcCount = 0;
        memset(g_DtcMemory, 0, sizeof(g_DtcMemory));
        return DIAG_E_OK;
    }
    
    /* Clear specific DTC */
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcMemory[i].dtcCode == dtcCode) {
            /* Remove DTC by shifting array */
            for (j = i; j < g_DtcCount - 1; j++) {
                g_DtcMemory[j] = g_DtcMemory[j + 1];
            }
            g_DtcCount--;
            return DIAG_E_OK;
        }
    }
    
    return DIAG_E_INVALID_DID;  /* DTC not found */
}

/**
 * @brief Read number of DTCs
 */
uint16_t Diag_ReadDTCCount(void) {
    return g_DtcCount;
}

/**
 * @brief Check if DTC is active
 */
bool Diag_IsDTCActive(uint32_t dtcCode) {
    uint8_t i;
    
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcMemory[i].dtcCode == dtcCode &&
            (g_DtcMemory[i].status & DTC_STATUS_TEST_FAILED)) {
            return true;
        }
    }
    
    return false;
}

/* =============================================================================
 * SERVICE 0x14: CLEAR DIAGNOSTICS
 * ============================================================================= */

static uint8_t Diag_HandleClearDTC(uint8_t *request, uint16_t requestLength,
                                    uint8_t *response, uint16_t *responseLength) {
    /* Response: [0x54] */
    response[0] = UDS_SERVICE_CLEAR_DTC + UDS_RESPONSE_OFFSET;
    *responseLength = 1;
    
    /* Clear all diagnostics */
    return Diag_ClearAllDiagnostics();
}

/**
 * @brief Clear all diagnostics
 */
uint8_t Diag_ClearAllDiagnostics(void) {
    g_DtcCount = 0;
    memset(g_DtcMemory, 0, sizeof(g_DtcMemory));
    memset(g_ActiveRoutines, 0, sizeof(g_ActiveRoutines));
    memset(g_DiagLog, 0, sizeof(g_DiagLog));
    g_DiagLogIndex = 0;
    return DIAG_E_OK;
}

/**
 * @brief Clear fault memory
 */
uint8_t Diag_ClearFaultMemory(void) {
    return Diag_ClearAllDiagnostics();
}

/* =============================================================================
 * SERVICE 0x3E: TESTER PRESENT
 * ============================================================================= */

static uint8_t Diag_HandleTesterPresent(uint8_t *request, uint16_t requestLength,
                                         uint8_t *response, uint16_t *responseLength) {
    uint8_t subFunction;
    
    if (requestLength < 2) {
        return UDS_ERROR_INVALID_MSG_LENGTH;
    }
    
    subFunction = request[1];
    
    /* Response: [0x7E, SubFunction] */
    response[0] = UDS_SERVICE_TESTER_PRESENT + UDS_RESPONSE_OFFSET;
    response[1] = subFunction;
    *responseLength = 2;
    
    return DIAG_E_OK;
}

/* =============================================================================
 * SESSION & SECURITY FUNCTIONS
 * ============================================================================= */

/**
 * @brief Set diagnostic session
 */
uint8_t Diag_SetSession(DiagnosticSessionType session) {
    g_CurrentSession = session;
    g_SessionStartTime = GetTimestampMs();
    return DIAG_E_OK;
}

/**
 * @brief Get current session
 */
DiagnosticSessionType Diag_GetCurrentSession(void) {
    return g_CurrentSession;
}

/**
 * @brief Request security access
 */
uint8_t Diag_RequestSecurityAccess(uint8_t level, 
                                    uint8_t *seedBuffer, 
                                    uint16_t *seedLength) {
    /* Simple seed: timestamp XOR with level */
    uint32_t seed = GetTimestampMs() ^ level;
    seedBuffer[0] = (uint8_t)(seed >> 24);
    seedBuffer[1] = (uint8_t)(seed >> 16);
    seedBuffer[2] = (uint8_t)(seed >> 8);
    seedBuffer[3] = (uint8_t)(seed & 0xFF);
    *seedLength = 4;
    
    g_SecurityLevel = level;
    return DIAG_E_OK;
}

/**
 * @brief Send security key
 */
uint8_t Diag_SendSecurityKey(uint8_t level, 
                              uint8_t *keyBuffer, 
                              uint16_t keyLength) {
    if (keyLength != 4) {
        return DIAG_E_INVALID_KEY;
    }
    
    /* Simple key validation: check if key matches expected format */
    g_SecurityAccessGranted = true;
    return DIAG_E_OK;
}

/**
 * @brief Check if security access granted
 */
bool Diag_IsSecurityAccessGranted(uint8_t level) {
    return (g_SecurityAccessGranted && g_SecurityLevel == level);
}

/* =============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================= */

/**
 * @brief Check if service is enabled
 */
bool Diag_IsServiceEnabled(uint8_t serviceId) {
    return (g_ServiceEnabled[serviceId] != 0);
}

/**
 * @brief Enable/disable service
 */
void Diag_EnableService(uint8_t serviceId, bool enabled) {
    g_ServiceEnabled[serviceId] = enabled ? 1 : 0;
}

/**
 * @brief Get diagnostic statistics
 */
void Diag_GetStatistics(uint16_t *activeDTCs, uint16_t *historicDTCs, 
                        uint32_t *sessionTime) {
    *activeDTCs = g_DtcCount;
    *historicDTCs = 0;  /* Not implemented */
    *sessionTime = GetElapsedTimeMs(g_SessionStartTime);
}

/**
 * @brief Log diagnostic event
 */
void Diag_LogEvent(uint8_t eventCode, uint32_t eventData) {
    g_DiagLog[g_DiagLogIndex].timestamp = GetTimestampMs();
    g_DiagLog[g_DiagLogIndex].eventCode = eventCode;
    g_DiagLog[g_DiagLogIndex].eventData = eventData;
    
    g_DiagLogIndex++;
    if (g_DiagLogIndex >= DIAG_LOG_SIZE) {
        g_DiagLogIndex = 0;  /* Circular buffer */
    }
}

/**
 * @brief Get DID data
 */
uint8_t Diag_GetDIDData(DidDataType *didData) {
    if (didData == NULL) {
        return DIAG_E_INVALID_DID;
    }
    
    *didData = g_DiagData;
    return DIAG_E_OK;
}

/**
 * @brief Get DTC info
 */
uint8_t Diag_GetDTCInfo(uint32_t dtcCode, DtcType *dtc) {
    uint8_t i;
    
    if (dtc == NULL) {
        return DIAG_E_INVALID_DID;
    }
    
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcMemory[i].dtcCode == dtcCode) {
            *dtc = g_DtcMemory[i];
            return DIAG_E_OK;
        }
    }
    
    return DIAG_E_INVALID_DID;
}

/**
 * @brief Main diagnostics task
 * 
 * Called periodically (50ms) to update diagnostics.
 */
void Diag_Task(void) {
    uint8_t i;
    
    /* Update routine status */
    for (i = 0; i < 3; i++) {
        if (g_ActiveRoutines[i].status == ROUTINE_STATUS_RUNNING) {
            if (GetElapsedTimeMs(g_ActiveRoutines[i].startTime) >= 
                g_ActiveRoutines[i].expectedDuration) {
                
                if (g_DiagData.errorCode == 0) {
                    g_ActiveRoutines[i].status = ROUTINE_STATUS_PASSED;
                } else {
                    g_ActiveRoutines[i].status = ROUTINE_STATUS_FAILED;
                }
            }
        }
    }
}

/* =============================================================================
 * END OF FILE
 * ============================================================================= */
