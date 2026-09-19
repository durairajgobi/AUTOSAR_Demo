/**
 * @file Dem.c
 * @brief DEM Module Implementation
 * 
 * Diagnostic Event Manager - Manages diagnostic events and DTCs.
 * 
 * @version 1.0
 * @date 2024
 */

#include "Dem.h"
#include <string.h>

/* =============================================================================
 * INTERNAL DATA STRUCTURES
 * ============================================================================= */

typedef enum {
    DEM_UNINIT = 0,
    DEM_INIT = 1,
    DEM_ENABLED = 2
} DemStatusType;

static DemStatusType g_DemStatus = DEM_UNINIT;

/* DTC Storage */
typedef struct {
    uint32 dtcCode;             /* DTC code */
    uint8 status;               /* Status byte */
    uint8 severity;             /* Severity (0-3) */
    uint32 occurrenceCount;     /* Occurrence count */
    uint32 firstOccurrenceTime; /* First occurrence time */
    uint32 lastOccurrenceTime;  /* Last occurrence time */
    uint16 failureCounter;      /* Failure counter */
    boolean isActive;           /* Active flag */
    uint8 snapshot[32];         /* Snapshot data */
    uint8 snapshotLength;       /* Snapshot length */
} DemDtcStorageType;

static DemDtcStorageType g_DtcStorage[DEM_DTC_COUNT];
static uint8 g_DtcCount = 0;

/* Event Status Storage */
typedef struct {
    uint8 eventStatus;          /* Event status byte */
    boolean occurred;           /* Occurred flag */
    uint32 lastOccurrenceTime;  /* Last occurrence time */
    uint8 snapshot[32];         /* Event snapshot */
    uint8 snapshotLength;       /* Snapshot length */
} DemEventStorageType;

#define DEM_EVENT_COUNT 10
static DemEventStorageType g_EventStorage[DEM_EVENT_COUNT];

/* Filter state */
static DemFilterType g_FilterType = DEM_FILTER_FOR_KEY_ON_CDTC;
static boolean g_FilterActive = FALSE;

/* =============================================================================
 * INITIALIZATION
 * ============================================================================= */

/**
 * @brief Initialize DEM
 */
Std_ReturnType Dem_Init(void) {
    uint8 i;
    
    if (g_DemStatus != DEM_UNINIT) {
        return E_NOT_OK;
    }
    
    /* Initialize DTC storage */
    for (i = 0; i < DEM_DTC_COUNT; i++) {
        g_DtcStorage[i].dtcCode = 0;
        g_DtcStorage[i].status = 0;
        g_DtcStorage[i].severity = 0;
        g_DtcStorage[i].occurrenceCount = 0;
        g_DtcStorage[i].failureCounter = 0;
        g_DtcStorage[i].isActive = FALSE;
        g_DtcStorage[i].snapshotLength = 0;
        memset(g_DtcStorage[i].snapshot, 0, sizeof(g_DtcStorage[i].snapshot));
    }
    
    /* Initialize event storage */
    for (i = 0; i < DEM_EVENT_COUNT; i++) {
        g_EventStorage[i].eventStatus = 0;
        g_EventStorage[i].occurred = FALSE;
        g_EventStorage[i].snapshotLength = 0;
    }
    
    g_DtcCount = 0;
    g_DemStatus = DEM_INIT;
    
    return E_OK;
}

/**
 * @brief Shutdown DEM
 */
Std_ReturnType Dem_Shutdown(void) {
    g_DemStatus = DEM_UNINIT;
    return E_OK;
}

/**
 * @brief Disable DEM
 */
void Dem_Disable(void) {
    if (g_DemStatus == DEM_ENABLED) {
        g_DemStatus = DEM_INIT;
    }
}

/**
 * @brief Enable DEM
 */
void Dem_Enable(void) {
    if (g_DemStatus == DEM_INIT) {
        g_DemStatus = DEM_ENABLED;
    }
}

/* =============================================================================
 * EVENT MANAGEMENT
 * ============================================================================= */

/**
 * @brief Set event status
 */
Std_ReturnType Dem_SetEventStatus(uint8 eventId, boolean eventStatus) {
    uint8 newStatus;
    
    if (g_DemStatus != DEM_ENABLED) {
        return E_NOT_OK;
    }
    
    if (eventId >= DEM_EVENT_COUNT) {
        return E_NOT_OK;
    }
    
    /* Update status */
    newStatus = eventStatus ? 0x01 : 0x00;  /* FAILED : PASSED */
    g_EventStorage[eventId].eventStatus = newStatus;
    g_EventStorage[eventId].occurred = TRUE;
    g_EventStorage[eventId].lastOccurrenceTime = 0;  /* Current time in real impl */
    
    /* If failed, potentially set DTC */
    if (eventStatus) {
        /* Map event to DTC and set it */
        uint32 dtcCode = Dem_EventToDtcCode(eventId);
        if (dtcCode != 0) {
            Dem_SetDTC(dtcCode, 2);  /* ERROR severity */
        }
    }
    
    return E_OK;
}

/**
 * @brief Get event status
 */
Std_ReturnType Dem_GetEventStatus(uint8 eventId, uint8 *eventStatus) {
    if (eventId >= DEM_EVENT_COUNT || eventStatus == NULL) {
        return E_NOT_OK;
    }
    
    *eventStatus = g_EventStorage[eventId].eventStatus;
    return E_OK;
}

/**
 * @brief Reset event status
 */
Std_ReturnType Dem_ResetEventStatus(uint8 eventId) {
    if (eventId >= DEM_EVENT_COUNT) {
        return E_NOT_OK;
    }
    
    g_EventStorage[eventId].eventStatus = 0;
    g_EventStorage[eventId].occurred = FALSE;
    
    return E_OK;
}

/**
 * @brief Get event occurred
 */
boolean Dem_GetEventOccurred(uint8 eventId) {
    if (eventId >= DEM_EVENT_COUNT) {
        return FALSE;
    }
    
    return g_EventStorage[eventId].occurred;
}

/* =============================================================================
 * DTC MANAGEMENT
 * ============================================================================= */

/**
 * @brief Set DTC
 */
Std_ReturnType Dem_SetDTC(uint32 dtcCode, uint8 severity) {
    uint8 i;
    
    if (g_DemStatus != DEM_ENABLED) {
        return E_NOT_OK;
    }
    
    /* Check if DTC already exists */
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].dtcCode == dtcCode) {
            /* Update existing DTC */
            g_DtcStorage[i].status |= 0x01;  /* Set FAILED bit */
            g_DtcStorage[i].occurrenceCount++;
            g_DtcStorage[i].lastOccurrenceTime = 0;  /* Current time in real impl */
            g_DtcStorage[i].isActive = TRUE;
            return E_OK;
        }
    }
    
    /* Add new DTC if space available */
    if (g_DtcCount < DEM_DTC_COUNT) {
        g_DtcStorage[g_DtcCount].dtcCode = dtcCode;
        g_DtcStorage[g_DtcCount].status = 0x01 | 0x04;  /* FAILED | CONFIRMED */
        g_DtcStorage[g_DtcCount].severity = severity;
        g_DtcStorage[g_DtcCount].occurrenceCount = 1;
        g_DtcStorage[g_DtcCount].firstOccurrenceTime = 0;
        g_DtcStorage[g_DtcCount].lastOccurrenceTime = 0;
        g_DtcStorage[g_DtcCount].failureCounter = 1;
        g_DtcStorage[g_DtcCount].isActive = TRUE;
        g_DtcCount++;
        return E_OK;
    }
    
    return E_NOT_OK;  /* DTC storage full */
}

/**
 * @brief Clear DTC
 */
Std_ReturnType Dem_ClearDTC(uint32 dtcCode) {
    uint8 i, j;
    
    if (dtcCode == 0) {
        /* Clear all DTCs */
        g_DtcCount = 0;
        memset(g_DtcStorage, 0, sizeof(g_DtcStorage));
        return E_OK;
    }
    
    /* Clear specific DTC */
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].dtcCode == dtcCode) {
            /* Remove DTC by shifting array */
            for (j = i; j < g_DtcCount - 1; j++) {
                g_DtcStorage[j] = g_DtcStorage[j + 1];
            }
            g_DtcCount--;
            return E_OK;
        }
    }
    
    return E_NOT_OK;  /* DTC not found */
}

/**
 * @brief Clear all DTCs
 */
Std_ReturnType Dem_ClearAllDTC(void) {
    return Dem_ClearDTC(0);
}

/**
 * @brief Get DTC info
 */
Std_ReturnType Dem_GetDTCInfo(uint32 dtcCode, DemDtcInfoType *dtcInfo) {
    uint8 i;
    
    if (dtcInfo == NULL) {
        return E_NOT_OK;
    }
    
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].dtcCode == dtcCode) {
            dtcInfo->dtcCode = g_DtcStorage[i].dtcCode;
            dtcInfo->dtcStatus = g_DtcStorage[i].status;
            dtcInfo->severity = g_DtcStorage[i].severity;
            dtcInfo->occurrenceCount = g_DtcStorage[i].occurrenceCount;
            dtcInfo->failureCounter = g_DtcStorage[i].failureCounter;
            dtcInfo->isActive = g_DtcStorage[i].isActive;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief Check if DTC is set
 */
boolean Dem_IsDTCSet(uint32 dtcCode) {
    uint8 i;
    
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].dtcCode == dtcCode) {
            return g_DtcStorage[i].isActive;
        }
    }
    
    return FALSE;
}

/**
 * @brief Get DTC count
 */
uint16 Dem_GetDTCCount(void) {
    return g_DtcCount;
}

/**
 * @brief Get DTC by index
 */
Std_ReturnType Dem_GetDTCByIndex(uint16 index, uint32 *dtcCode) {
    if (index >= g_DtcCount || dtcCode == NULL) {
        return E_NOT_OK;
    }
    
    *dtcCode = g_DtcStorage[index].dtcCode;
    return E_OK;
}

/* =============================================================================
 * FILTER AND SEARCH
 * ============================================================================= */

/**
 * @brief Set DTC filter
 */
Std_ReturnType Dem_SetDTCFilter(DemFilterType filterType) {
    g_FilterType = filterType;
    g_FilterActive = TRUE;
    return E_OK;
}

/**
 * @brief Get filtered DTC
 */
Std_ReturnType Dem_GetFilteredDTC(uint32 *dtcBuffer, uint16 bufferSize, 
                                   uint16 *dtcCount) {
    uint8 i;
    uint16 count = 0;
    
    if (dtcBuffer == NULL || dtcCount == NULL) {
        return E_NOT_OK;
    }
    
    if (!g_FilterActive) {
        *dtcCount = 0;
        return E_OK;
    }
    
    /* Apply filter logic */
    for (i = 0; i < g_DtcCount && count < bufferSize; i++) {
        if (Dem_ApplyFilter(&g_DtcStorage[i])) {
            dtcBuffer[count] = g_DtcStorage[i].dtcCode;
            count++;
        }
    }
    
    *dtcCount = count;
    return E_OK;
}

/**
 * @brief Find DTC
 */
boolean Dem_FindDTC(uint32 dtcCode) {
    uint8 i;
    
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].dtcCode == dtcCode) {
            return TRUE;
        }
    }
    
    return FALSE;
}

/* =============================================================================
 * SNAPSHOT AND EXTENDED DATA
 * ============================================================================= */

/**
 * @brief Store event snapshot
 */
Std_ReturnType Dem_StoreEventSnapshot(uint8 eventId, const uint8 *data, 
                                       uint8 length) {
    if (eventId >= DEM_EVENT_COUNT || data == NULL) {
        return E_NOT_OK;
    }
    
    if (length > sizeof(g_EventStorage[eventId].snapshot)) {
        length = sizeof(g_EventStorage[eventId].snapshot);
    }
    
    memcpy(g_EventStorage[eventId].snapshot, data, length);
    g_EventStorage[eventId].snapshotLength = length;
    
    return E_OK;
}

/**
 * @brief Get event snapshot
 */
Std_ReturnType Dem_GetEventSnapshot(uint8 eventId, uint8 *data, uint8 *length) {
    if (eventId >= DEM_EVENT_COUNT || data == NULL || length == NULL) {
        return E_NOT_OK;
    }
    
    if (g_EventStorage[eventId].snapshotLength == 0) {
        return E_NOT_OK;
    }
    
    memcpy(data, g_EventStorage[eventId].snapshot, 
           g_EventStorage[eventId].snapshotLength);
    *length = g_EventStorage[eventId].snapshotLength;
    
    return E_OK;
}

/**
 * @brief Store extended data
 */
Std_ReturnType Dem_StoreExtendedData(uint8 eventId, const uint8 *data, 
                                      uint8 length) {
    /* Extended data not implemented in this basic version */
    return E_OK;
}

/* =============================================================================
 * FAILURE AND FREEZE FRAME
 * ============================================================================= */

/**
 * @brief Increment failure counter
 */
Std_ReturnType Dem_IncrementFailureCounter(uint8 eventId) {
    if (eventId >= DEM_EVENT_COUNT) {
        return E_NOT_OK;
    }
    
    /* Map event to DTC and increment its failure counter */
    uint32 dtcCode = Dem_EventToDtcCode(eventId);
    if (dtcCode == 0) {
        return E_NOT_OK;
    }
    
    uint8 i;
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].dtcCode == dtcCode) {
            if (g_DtcStorage[i].failureCounter < 0xFFFF) {
                g_DtcStorage[i].failureCounter++;
            }
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief Get failure counter
 */
Std_ReturnType Dem_GetFailureCounter(uint8 eventId, uint16 *counter) {
    if (eventId >= DEM_EVENT_COUNT || counter == NULL) {
        return E_NOT_OK;
    }
    
    /* Get counter from associated DTC */
    uint32 dtcCode = Dem_EventToDtcCode(eventId);
    if (dtcCode == 0) {
        return E_NOT_OK;
    }
    
    uint8 i;
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].dtcCode == dtcCode) {
            *counter = g_DtcStorage[i].failureCounter;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief Store freeze frame
 */
Std_ReturnType Dem_StoreFreezeFrame(uint32 dtcCode, const uint8 *data, 
                                     uint8 length) {
    uint8 i;
    
    if (data == NULL) {
        return E_NOT_OK;
    }
    
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].dtcCode == dtcCode) {
            if (length > sizeof(g_DtcStorage[i].snapshot)) {
                length = sizeof(g_DtcStorage[i].snapshot);
            }
            memcpy(g_DtcStorage[i].snapshot, data, length);
            g_DtcStorage[i].snapshotLength = length;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief Get freeze frame
 */
Std_ReturnType Dem_GetFreezeFrame(uint32 dtcCode, uint8 *data, uint8 *length) {
    uint8 i;
    
    if (data == NULL || length == NULL) {
        return E_NOT_OK;
    }
    
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].dtcCode == dtcCode) {
            if (g_DtcStorage[i].snapshotLength == 0) {
                return E_NOT_OK;
            }
            memcpy(data, g_DtcStorage[i].snapshot, g_DtcStorage[i].snapshotLength);
            *length = g_DtcStorage[i].snapshotLength;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/* =============================================================================
 * MEMORY AND STATISTICS
 * ============================================================================= */

/**
 * @brief Get memory usage
 */
Std_ReturnType Dem_GetMemoryUsage(uint32 *totalSize, uint32 *usedSize) {
    if (totalSize == NULL || usedSize == NULL) {
        return E_NOT_OK;
    }
    
    *totalSize = sizeof(g_DtcStorage) + sizeof(g_EventStorage);
    *usedSize = (g_DtcCount * sizeof(DemDtcStorageType));
    
    return E_OK;
}

/**
 * @brief Get statistics
 */
Std_ReturnType Dem_GetStatistics(uint16 *activeDTCs, uint16 *historicDTCs,
                                  uint16 *totalEvents) {
    uint8 i;
    uint16 active = 0, historic = 0;
    
    if (activeDTCs == NULL || historicDTCs == NULL || totalEvents == NULL) {
        return E_NOT_OK;
    }
    
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].isActive) {
            active++;
        } else {
            historic++;
        }
    }
    
    *activeDTCs = active;
    *historicDTCs = historic;
    *totalEvents = DEM_EVENT_COUNT;
    
    return E_OK;
}

/**
 * @brief Clear fault memory
 */
Std_ReturnType Dem_ClearFaultMemory(void) {
    return Dem_ClearAllDTC();
}

/**
 * @brief Clear pending DTC
 */
Std_ReturnType Dem_ClearPendingDTC(void) {
    uint8 i;
    
    for (i = 0; i < g_DtcCount; i++) {
        g_DtcStorage[i].status &= ~0x02;  /* Clear pending bit */
    }
    
    return E_OK;
}

/* =============================================================================
 * TIMING AND MONITORING
 * ============================================================================= */

/**
 * @brief DEM main function
 */
void Dem_MainFunction(void) {
    /* Update DTC status, handle aging, etc. */
    uint8 i;
    
    if (g_DemStatus != DEM_ENABLED) {
        return;
    }
    
    for (i = 0; i < g_DtcCount; i++) {
        /* Update DTC aging, timeouts, etc. */
    }
}

/**
 * @brief Set aging threshold
 */
void Dem_SetAgingThreshold(uint32 threshold) {
    /* Implement aging logic */
}

/**
 * @brief Age DTC
 */
void Dem_AgeDTC(uint32 dtcCode) {
    uint8 i;
    
    for (i = 0; i < g_DtcCount; i++) {
        if (g_DtcStorage[i].dtcCode == dtcCode) {
            /* Decrement aging counter */
        }
    }
}

/* =============================================================================
 * CONTROL
 * ============================================================================= */

/**
 * @brief Set operation cycle
 */
void Dem_SetOperationCycle(uint8 cycleId, boolean started) {
    /* Track operation cycles for aging */
}

/**
 * @brief Check operation cycle
 */
boolean Dem_IsOperationCycleActive(uint8 cycleId) {
    return FALSE;  /* Not implemented */
}

/**
 * @brief Get DEM status
 */
uint8 Dem_GetStatus(void) {
    return (uint8)g_DemStatus;
}

/* =============================================================================
 * INTERNAL HELPER FUNCTIONS
 * ============================================================================= */

/**
 * @brief Map event ID to DTC code
 */
static uint32 Dem_EventToDtcCode(uint8 eventId) {
    switch (eventId) {
        case 0:
            return DEM_DTC_OVERCURRENT_ERROR;
        case 1:
            return DEM_DTC_TIMEOUT_ERROR;
        case 2:
            return DEM_DTC_SENSOR_ERROR;
        case 3:
            return DEM_DTC_MOTOR_ERROR;
        case 4:
            return DEM_DTC_JAM_DETECTED;
        default:
            return 0;
    }
}

/**
 * @brief Apply active filter to DTC
 */
static boolean Dem_ApplyFilter(DemDtcStorageType *dtc) {
    if (dtc == NULL) {
        return FALSE;
    }
    
    switch (g_FilterType) {
        case DEM_FILTER_FOR_KEY_ON_CDTC:
            return (dtc->status & 0x01) ? TRUE : FALSE;  /* FAILED */
        case DEM_FILTER_FOR_KEY_ON_SDTC:
            return dtc->isActive;
        default:
            return TRUE;
    }
}

/* =============================================================================
 * END OF FILE
 * ============================================================================= */
