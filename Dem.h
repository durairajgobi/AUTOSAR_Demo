/**
 * @file Dem.h
 * @brief DEM Module - Diagnostic Event Manager
 * 
 * AUTOSAR DEM Layer - Manages diagnostic events and DTCs.
 * Stores fault information, supports DCM queries.
 * 
 * AUTOSAR 4.3.1 Compliant
 * 
 * @version 1.0
 * @date 2024
 */

#ifndef DEM_H
#define DEM_H

#include "Rte_Type.h"

/* =============================================================================
 * DTC DEFINITIONS
 * ============================================================================= */

#define DEM_DTC_OVERCURRENT_ERROR       0xC10001
#define DEM_DTC_TIMEOUT_ERROR           0xC10002
#define DEM_DTC_SENSOR_ERROR            0xC10003
#define DEM_DTC_MOTOR_ERROR             0xC10004
#define DEM_DTC_JAM_DETECTED            0xC10005

#define DEM_DTC_COUNT                   5

/* =============================================================================
 * EVENT STATUS DEFINITIONS
 * ============================================================================= */

typedef enum {
    DEM_EVENT_STATUS_PASSED = 0x00,
    DEM_EVENT_STATUS_FAILED = 0x01,
    DEM_EVENT_STATUS_PASSED_SINCE_LAST_CLEAR = 0x04,
    DEM_EVENT_STATUS_FAILED_SINCE_LAST_CLEAR = 0x08,
    DEM_EVENT_STATUS_TESTED_SINCE_LAST_CLEAR = 0x10,
    DEM_EVENT_STATUS_TESTED = 0x20,
    DEM_EVENT_STATUS_CDTC = 0x40,
    DEM_EVENT_STATUS_SDTC = 0x80
} DemEventStatusType;

/* =TC Status Availability Mask */
#define DEM_STATUS_AVAILABILITY_MASK    0xF3

/* =============================================================================
 * FILTER TYPES
 * ============================================================================= */

typedef enum {
    DEM_FILTER_FOR_KEY_OFF_CDTC = 0x00,
    DEM_FILTER_FOR_KEY_OFF_SDTC = 0x01,
    DEM_FILTER_FOR_KEY_ON_CDTC = 0x02,
    DEM_FILTER_FOR_KEY_ON_SDTC = 0x03,
    DEM_FILTER_FOR_KEY_ON_SDTC_HIGH = 0x04,
    DEM_FILTER_FOR_PERMANENT_CDTC = 0x05,
    DEM_FILTER_FOR_PERMANENT_SDTC = 0x06
} DemFilterType;

/* =============================================================================
 * DTC STATUS INFORMATION
 * ============================================================================= */

typedef struct {
    uint32 dtcCode;             /* DTC code */
    uint8 dtcStatus;            /* Status byte */
    uint8 severity;             /* Severity level (0-3) */
    uint32 occurrenceCount;     /* Number of occurrences */
    uint32 firstOccurrenceTime; /* First occurrence timestamp */
    uint32 lastOccurrenceTime;  /* Last occurrence timestamp */
    uint16 failureCounter;      /* Failure counter */
    boolean isActive;           /* Active flag */
} DemDtcInfoType;

/* =============================================================================
 * INITIALIZATION
 * ============================================================================= */

/**
 * @brief Initialize DEM module
 * 
 * @return E_OK if successful
 */
Std_ReturnType Dem_Init(void);

/**
 * @brief Shutdown DEM module
 * 
 * @return E_OK if successful
 */
Std_ReturnType Dem_Shutdown(void);

/**
 * @brief Disable DEM component
 */
void Dem_Disable(void);

/**
 * @brief Enable DEM component
 */
void Dem_Enable(void);

/* =============================================================================
 * EVENT MANAGEMENT
 * ============================================================================= */

/**
 * @brief Set event status
 * 
 * Reports the status of a diagnostic event.
 * 
 * @param[in] eventId: Event identifier
 * @param[in] eventStatus: true=failed, false=passed
 * @return E_OK if successful
 */
Std_ReturnType Dem_SetEventStatus(uint8 eventId, boolean eventStatus);

/**
 * @brief Get event status
 * 
 * @param[in] eventId: Event identifier
 * @param[out] eventStatus: Pointer to status byte
 * @return E_OK if successful
 */
Std_ReturnType Dem_GetEventStatus(uint8 eventId, uint8 *eventStatus);

/**
 * @brief Reset event status
 * 
 * @param[in] eventId: Event identifier
 * @return E_OK if successful
 */
Std_ReturnType Dem_ResetEventStatus(uint8 eventId);

/**
 * @brief Get event occurred flag
 * 
 * Checks if an event has occurred since last clear.
 * 
 * @param[in] eventId: Event identifier
 * @return true if event occurred, false otherwise
 */
boolean Dem_GetEventOccurred(uint8 eventId);

/* =============================================================================
 * DTC MANAGEMENT
 * ============================================================================= */

/**
 * @brief Set DTC
 * 
 * Reports a DTC (Diagnostic Trouble Code).
 * 
 * @param[in] dtcCode: DTC code (e.g., 0xC10001)
 * @param[in] severity: Severity level
 * @return E_OK if successful
 */
Std_ReturnType Dem_SetDTC(uint32 dtcCode, uint8 severity);

/**
 * @brief Clear DTC
 * 
 * @param[in] dtcCode: DTC code (0 to clear all)
 * @return E_OK if successful
 */
Std_ReturnType Dem_ClearDTC(uint32 dtcCode);

/**
 * @brief Clear all DTCs
 * 
 * @return E_OK if successful
 */
Std_ReturnType Dem_ClearAllDTC(void);

/**
 * @brief Get DTC information
 * 
 * @param[in] dtcCode: DTC code
 * @param[out] dtcInfo: Pointer to DTC information structure
 * @return E_OK if DTC found
 */
Std_ReturnType Dem_GetDTCInfo(uint32 dtcCode, DemDtcInfoType *dtcInfo);

/**
 * @brief Check if DTC is set
 * 
 * @param[in] dtcCode: DTC code
 * @return true if DTC is set, false otherwise
 */
boolean Dem_IsDTCSet(uint32 dtcCode);

/**
 * @brief Get number of DTCs
 * 
 * @return Number of active DTCs
 */
uint16 Dem_GetDTCCount(void);

/**
 * @brief Get DTC at index
 * 
 * @param[in] index: Index in DTC list
 * @param[out] dtcCode: Pointer to receive DTC code
 * @return E_OK if successful
 */
Std_ReturnType Dem_GetDTCByIndex(uint16 index, uint32 *dtcCode);

/* =============================================================================
 * FILTER AND SEARCH
 * ============================================================================= */

/**
 * @brief Set DTC filter
 * 
 * @param[in] filterType: Filter type
 * @return E_OK if successful
 */
Std_ReturnType Dem_SetDTCFilter(DemFilterType filterType);

/**
 * @brief Get filtered DTCs
 * 
 * @param[out] dtcBuffer: Buffer to receive DTC codes
 * @param[in] bufferSize: Size of buffer
 * @param[out] dtcCount: Actual number of DTCs returned
 * @return E_OK if successful
 */
Std_ReturnType Dem_GetFilteredDTC(uint32 *dtcBuffer, uint16 bufferSize, 
                                   uint16 *dtcCount);

/**
 * @brief Find DTC by code
 * 
 * @param[in] dtcCode: DTC code to find
 * @return true if DTC found, false otherwise
 */
boolean Dem_FindDTC(uint32 dtcCode);

/* =============================================================================
 * SNAPSHOT AND EXTENDED DATA
 * ============================================================================= */

/**
 * @brief Store event snapshot
 * 
 * Stores diagnostic data snapshot for a failed event.
 * 
 * @param[in] eventId: Event identifier
 * @param[in] data: Pointer to snapshot data
 * @param[in] length: Data length
 * @return E_OK if successful
 */
Std_ReturnType Dem_StoreEventSnapshot(uint8 eventId, const uint8 *data, 
                                       uint8 length);

/**
 * @brief Get event snapshot
 * 
 * @param[in] eventId: Event identifier
 * @param[out] data: Buffer for snapshot data
 * @param[out] length: Length of snapshot data
 * @return E_OK if snapshot available
 */
Std_ReturnType Dem_GetEventSnapshot(uint8 eventId, uint8 *data, uint8 *length);

/**
 * @brief Store extended data
 * 
 * @param[in] eventId: Event identifier
 * @param[in] data: Pointer to extended data
 * @param[in] length: Data length
 * @return E_OK if successful
 */
Std_ReturnType Dem_StoreExtendedData(uint8 eventId, const uint8 *data, 
                                      uint8 length);

/* =============================================================================
 * FAILURE AND FREEZE FRAME
 * ============================================================================= */

/**
 * @brief Increment failure counter
 * 
 * @param[in] eventId: Event identifier
 * @return E_OK if successful
 */
Std_ReturnType Dem_IncrementFailureCounter(uint8 eventId);

/**
 * @brief Get failure counter
 * 
 * @param[in] eventId: Event identifier
 * @param[out] counter: Pointer to receive counter value
 * @return E_OK if successful
 */
Std_ReturnType Dem_GetFailureCounter(uint8 eventId, uint16 *counter);

/**
 * @brief Store freeze frame
 * 
 * Stores data snapshot when DTC occurs.
 * 
 * @param[in] dtcCode: DTC code
 * @param[in] data: Pointer to freeze frame data
 * @param[in] length: Data length
 * @return E_OK if successful
 */
Std_ReturnType Dem_StoreFreezeFrame(uint32 dtcCode, const uint8 *data, 
                                     uint8 length);

/**
 * @brief Get freeze frame
 * 
 * @param[in] dtcCode: DTC code
 * @param[out] data: Buffer for freeze frame
 * @param[out] length: Length of data
 * @return E_OK if freeze frame available
 */
Std_ReturnType Dem_GetFreezeFrame(uint32 dtcCode, uint8 *data, uint8 *length);

/* =============================================================================
 * MEMORY AND STATISTICS
 * ============================================================================= */

/**
 * @brief Get DEM memory usage
 * 
 * @param[out] totalSize: Total memory size
 * @param[out] usedSize: Used memory size
 * @return E_OK if successful
 */
Std_ReturnType Dem_GetMemoryUsage(uint32 *totalSize, uint32 *usedSize);

/**
 * @brief Get DEM statistics
 * 
 * @param[out] activeDTCs: Number of active DTCs
 * @param[out] historicDTCs: Number of historic DTCs
 * @param[out] totalEvents: Total number of events
 * @return E_OK if successful
 */
Std_ReturnType Dem_GetStatistics(uint16 *activeDTCs, uint16 *historicDTCs,
                                  uint16 *totalEvents);

/**
 * @brief Clear fault memory
 * 
 * Clears all DTC information and fault memory.
 * 
 * @return E_OK if successful
 */
Std_ReturnType Dem_ClearFaultMemory(void);

/**
 * @brief Clear pending DTCs
 * 
 * @return E_OK if successful
 */
Std_ReturnType Dem_ClearPendingDTC(void);

/* =============================================================================
 * TIMING AND MONITORING
 * ============================================================================= */

/**
 * @brief DEM main function
 * 
 * Called periodically to update DTC status.
 */
void Dem_MainFunction(void);

/**
 * @brief Set DTC aging threshold
 * 
 * @param[in] threshold: Threshold in main function calls
 */
void Dem_SetAgingThreshold(uint32 threshold);

/**
 * @brief Age DTC
 * 
 * Reduces DTC aging counter.
 * 
 * @param[in] dtcCode: DTC code
 */
void Dem_AgeDTC(uint32 dtcCode);

/* =============================================================================
 * CONTROL
 * ============================================================================= */

/**
 * @brief Set operation cycle
 * 
 * @param[in] cycleId: Cycle identifier
 * @param[in] started: true=started, false=ended
 */
void Dem_SetOperationCycle(uint8 cycleId, boolean started);

/**
 * @brief Check operation cycle
 * 
 * @param[in] cycleId: Cycle identifier
 * @return true if cycle active
 */
boolean Dem_IsOperationCycleActive(uint8 cycleId);

/**
 * @brief Get DEM status
 * 
 * @return DEM status (0=uninitialized, 1=initialized, 2=enabled)
 */
uint8 Dem_GetStatus(void);

/* =============================================================================
 * END OF FILE
 * ============================================================================= */

#endif /* DEM_H */
