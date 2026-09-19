/**
 * @file CanIf.c
 * @brief CAN Interface Implementation
 * 
 * AUTOSAR CanIf Layer - CAN message routing and filtering.
 * 
 * @version 1.0
 * @date 2024
 */

#include "CanIf.h"
#include "CanDrv.h"
#include "Com.h"
#include <string.h>

/* =============================================================================
 * INTERNAL DATA STRUCTURES
 * ============================================================================= */

typedef enum {
    CANIF_UNINIT = 0,
    CANIF_INIT = 1,
    CANIF_STARTED = 2
} CanIfStatusType;

static CanIfStatusType g_CanIfStatus = CANIF_UNINIT;
static CanControllerStateType g_ControllerState = CAN_CTRL_UNINIT;

/* CAN Message Queue */
#define CANIF_TX_QUEUE_SIZE 16
#define CANIF_RX_QUEUE_SIZE 32

typedef struct {
    CanFrameType frame;
    uint32 timestamp;
    uint8 status;  /* 0=pending, 1=sent, 2=error */
} CanIfTxQueueEntry;

typedef struct {
    CanFrameType frame;
    uint32 timestamp;
    boolean isNew;
} CanIfRxQueueEntry;

static CanIfTxQueueEntry g_TxQueue[CANIF_TX_QUEUE_SIZE];
static uint8 g_TxQueueIndex = 0;

static CanIfRxQueueEntry g_RxQueue[CANIF_RX_QUEUE_SIZE];
static uint8 g_RxQueueIndex = 0;
static uint8 g_RxQueueWrite = 0;

/* Receive Filters */
#define CANIF_FILTER_COUNT 16

typedef struct {
    uint16 canId;
    uint16 mask;
    boolean isActive;
} CanIfFilter;

static CanIfFilter g_RxFilters[CANIF_FILTER_COUNT];
static uint8 g_FilterCount = 0;

/* Statistics */
typedef struct {
    uint32 txCount;
    uint32 rxCount;
    uint32 errorCount;
    uint16 errorCounter;
    boolean isBusOff;
} CanIfStats;

static CanIfStats g_Statistics = {0};

/* Configuration */
static uint16 g_BaudRate = 500;  /* 500 kbps */
static uint8 g_ControllerMode = 1;  /* Normal mode */

/* Callbacks */
typedef struct {
    uint16 canId;
    CanTxIndicationCallback txCallback;
    CanRxIndicationCallback rxCallback;
} CanIfCallback;

#define CANIF_CALLBACK_COUNT 8
static CanIfCallback g_Callbacks[CANIF_CALLBACK_COUNT];
static uint8 g_CallbackCount = 0;
static CanErrorCallback g_ErrorCallback = NULL;

/* =============================================================================
 * INITIALIZATION
 * ============================================================================= */

/**
 * @brief Initialize CanIf
 */
Std_ReturnType CanIf_Init(void) {
    uint8 i;
    
    if (g_CanIfStatus != CANIF_UNINIT) {
        return E_NOT_OK;
    }
    
    /* Initialize CAN driver */
    if (CanDrv_Init() != E_OK) {
        return E_NOT_OK;
    }
    
    /* Initialize queue structures */
    memset(g_TxQueue, 0, sizeof(g_TxQueue));
    memset(g_RxQueue, 0, sizeof(g_RxQueue));
    memset(g_RxFilters, 0, sizeof(g_RxFilters));
    memset(g_Callbacks, 0, sizeof(g_Callbacks));
    
    /* Add default receive filters for known CAN IDs */
    CanIf_AddReceiveFilter(0x100, 0xFFFF);  /* DoorCommand */
    CanIf_AddReceiveFilter(0x101, 0xFFFF);  /* DoorStatus */
    CanIf_AddReceiveFilter(0x7D0, 0xFFFF);  /* Diagnostics */
    
    /* Initialize statistics */
    g_Statistics.txCount = 0;
    g_Statistics.rxCount = 0;
    g_Statistics.errorCount = 0;
    g_Statistics.errorCounter = 0;
    g_Statistics.isBusOff = FALSE;
    
    g_CanIfStatus = CANIF_INIT;
    g_ControllerState = CAN_CTRL_STOPPED;
    
    return E_OK;
}

/**
 * @brief Shutdown CanIf
 */
Std_ReturnType CanIf_Shutdown(void) {
    CanDrv_Shutdown();
    g_CanIfStatus = CANIF_UNINIT;
    g_ControllerState = CAN_CTRL_UNINIT;
    return E_OK;
}

/**
 * @brief Start CanIf
 */
Std_ReturnType CanIf_Start(void) {
    if (g_CanIfStatus != CANIF_INIT) {
        return E_NOT_OK;
    }
    
    /* Start CAN controller */
    if (CanDrv_Start() != E_OK) {
        return E_NOT_OK;
    }
    
    g_CanIfStatus = CANIF_STARTED;
    g_ControllerState = CAN_CTRL_STARTED;
    
    return E_OK;
}

/**
 * @brief Stop CanIf
 */
Std_ReturnType CanIf_Stop(void) {
    CanDrv_Stop();
    g_CanIfStatus = CANIF_INIT;
    g_ControllerState = CAN_CTRL_STOPPED;
    return E_OK;
}

/* =============================================================================
 * TRANSMISSION
 * ============================================================================= */

/**
 * @brief Transmit CAN message
 */
Std_ReturnType CanIf_Transmit(uint16 canId, const uint8 *data, uint8 dlc) {
    CanFrameType frame;
    
    if (g_CanIfStatus != CANIF_STARTED) {
        return E_NOT_OK;
    }
    
    if (data == NULL || dlc > 8) {
        return E_NOT_OK;
    }
    
    /* Create frame */
    frame.canId = canId;
    frame.dlc = dlc;
    frame.isExtended = FALSE;
    frame.isRemote = FALSE;
    memcpy(frame.data, data, dlc);
    
    /* Add to transmission queue */
    if (g_TxQueueIndex >= CANIF_TX_QUEUE_SIZE) {
        return E_NOT_OK;  /* Queue full */
    }
    
    g_TxQueue[g_TxQueueIndex].frame = frame;
    g_TxQueue[g_TxQueueIndex].status = 0;  /* Pending */
    g_TxQueue[g_TxQueueIndex].timestamp = 0;
    g_TxQueueIndex++;
    
    return E_OK;
}

/**
 * @brief Transmit frame
 */
Std_ReturnType CanIf_TransmitFrame(const CanFrameType *frame) {
    if (frame == NULL) {
        return E_NOT_OK;
    }
    
    return CanIf_Transmit(frame->canId, frame->data, frame->dlc);
}

/**
 * @brief Get transmission status
 */
uint8 CanIf_GetTransmitStatus(uint16 canId) {
    uint8 i;
    
    for (i = 0; i < g_TxQueueIndex; i++) {
        if (g_TxQueue[i].frame.canId == canId) {
            return g_TxQueue[i].status;
        }
    }
    
    return 0;  /* Not found */
}

/**
 * @brief Cancel transmission
 */
Std_ReturnType CanIf_CancelTransmit(uint16 canId) {
    uint8 i, j;
    
    for (i = 0; i < g_TxQueueIndex; i++) {
        if (g_TxQueue[i].frame.canId == canId) {
            /* Remove by shifting */
            for (j = i; j < g_TxQueueIndex - 1; j++) {
                g_TxQueue[j] = g_TxQueue[j + 1];
            }
            g_TxQueueIndex--;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/* =============================================================================
 * RECEPTION
 * ============================================================================= */

/**
 * @brief Receive CAN message
 */
Std_ReturnType CanIf_Receive(uint16 canId, uint8 *data, uint8 *dlc) {
    uint8 i;
    
    if (data == NULL || dlc == NULL) {
        return E_NOT_OK;
    }
    
    /* Search for message in receive queue */
    for (i = g_RxQueueIndex; i < g_RxQueueWrite; i++) {
        if (g_RxQueue[i].frame.canId == canId && g_RxQueue[i].isNew) {
            memcpy(data, g_RxQueue[i].frame.data, g_RxQueue[i].frame.dlc);
            *dlc = g_RxQueue[i].frame.dlc;
            g_RxQueue[i].isNew = FALSE;
            return E_OK;
        }
    }
    
    return E_NOT_OK;  /* Message not found */
}

/**
 * @brief Receive frame
 */
Std_ReturnType CanIf_ReceiveFrame(CanFrameType *frame) {
    if (frame == NULL) {
        return E_NOT_OK;
    }
    
    if (g_RxQueueIndex < g_RxQueueWrite) {
        *frame = g_RxQueue[g_RxQueueIndex].frame;
        g_RxQueue[g_RxQueueIndex].isNew = FALSE;
        g_RxQueueIndex++;
        return E_OK;
    }
    
    return E_NOT_OK;
}

/**
 * @brief Get reception status
 */
uint8 CanIf_GetReceiveStatus(uint16 canId) {
    return CanIf_IsMessageAvailable(canId) ? 1 : 0;
}

/**
 * @brief Check if message available
 */
boolean CanIf_IsMessageAvailable(uint16 canId) {
    uint8 i;
    
    for (i = g_RxQueueIndex; i < g_RxQueueWrite; i++) {
        if (g_RxQueue[i].frame.canId == canId) {
            return TRUE;
        }
    }
    
    return FALSE;
}

/* =============================================================================
 * MESSAGE FILTERING
 * ============================================================================= */

/**
 * @brief Add receive filter
 */
Std_ReturnType CanIf_AddReceiveFilter(uint16 canId, uint16 mask) {
    if (g_FilterCount >= CANIF_FILTER_COUNT) {
        return E_NOT_OK;
    }
    
    g_RxFilters[g_FilterCount].canId = canId;
    g_RxFilters[g_FilterCount].mask = mask;
    g_RxFilters[g_FilterCount].isActive = TRUE;
    g_FilterCount++;
    
    return E_OK;
}

/**
 * @brief Remove receive filter
 */
Std_ReturnType CanIf_RemoveReceiveFilter(uint16 canId) {
    uint8 i, j;
    
    for (i = 0; i < g_FilterCount; i++) {
        if (g_RxFilters[i].canId == canId) {
            for (j = i; j < g_FilterCount - 1; j++) {
                g_RxFilters[j] = g_RxFilters[j + 1];
            }
            g_FilterCount--;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief Clear all filters
 */
Std_ReturnType CanIf_ClearAllFilters(void) {
    memset(g_RxFilters, 0, sizeof(g_RxFilters));
    g_FilterCount = 0;
    return E_OK;
}

/**
 * @brief Get filter count
 */
uint8 CanIf_GetFilterCount(void) {
    return g_FilterCount;
}

/* =============================================================================
 * CONFIGURATION
 * ============================================================================= */

/**
 * @brief Set baud rate
 */
Std_ReturnType CanIf_SetBaudRate(uint16 baudRate) {
    if (g_ControllerState != CAN_CTRL_STOPPED) {
        return E_NOT_OK;
    }
    
    g_BaudRate = baudRate;
    return CanDrv_SetBaudRate(baudRate);
}

/**
 * @brief Get baud rate
 */
uint16 CanIf_GetBaudRate(void) {
    return g_BaudRate;
}

/**
 * @brief Set controller mode
 */
Std_ReturnType CanIf_SetControllerMode(uint8 mode) {
    g_ControllerMode = mode;
    return E_OK;
}

/* =============================================================================
 * CONTROLLER MANAGEMENT
 * ============================================================================= */

/**
 * @brief Get controller state
 */
CanControllerStateType CanIf_GetControllerState(void) {
    return g_ControllerState;
}

/**
 * @brief Disable controller
 */
Std_ReturnType CanIf_DisableController(void) {
    return CanDrv_DisableController();
}

/**
 * @brief Enable controller
 */
Std_ReturnType CanIf_EnableController(void) {
    return CanDrv_EnableController();
}

/**
 * @brief Reset controller
 */
Std_ReturnType CanIf_ResetController(void) {
    return CanDrv_Reset();
}

/**
 * @brief Wakeup
 */
Std_ReturnType CanIf_Wakeup(void) {
    return CanDrv_Wakeup();
}

/**
 * @brief Go to sleep
 */
Std_ReturnType CanIf_GoToSleep(void) {
    return CanDrv_Sleep();
}

/* =============================================================================
 * STATUS AND MONITORING
 * ============================================================================= */

/**
 * @brief Get CanIf status
 */
uint8 CanIf_GetStatus(void) {
    return (uint8)g_CanIfStatus;
}

/**
 * @brief Get bus statistics
 */
Std_ReturnType CanIf_GetBusStatistics(uint32 *txCount, uint32 *rxCount, 
                                       uint32 *errorCount) {
    if (txCount == NULL || rxCount == NULL || errorCount == NULL) {
        return E_NOT_OK;
    }
    
    *txCount = g_Statistics.txCount;
    *rxCount = g_Statistics.rxCount;
    *errorCount = g_Statistics.errorCount;
    
    return E_OK;
}

/**
 * @brief Get error counter
 */
uint16 CanIf_GetErrorCounter(void) {
    return g_Statistics.errorCounter;
}

/**
 * @brief Check bus-off
 */
boolean CanIf_IsBusOff(void) {
    return g_Statistics.isBusOff;
}

/**
 * @brief Check error frame
 */
boolean CanIf_IsErrorFrameReceived(void) {
    return (g_Statistics.errorCounter > 0) ? TRUE : FALSE;
}

/* =============================================================================
 * ROUTING AND CALLBACKS
 * ============================================================================= */

/**
 * @brief Register TX indication
 */
Std_ReturnType CanIf_RegisterTxIndication(uint16 canId, 
                                           CanTxIndicationCallback callback) {
    if (g_CallbackCount >= CANIF_CALLBACK_COUNT || callback == NULL) {
        return E_NOT_OK;
    }
    
    g_Callbacks[g_CallbackCount].canId = canId;
    g_Callbacks[g_CallbackCount].txCallback = callback;
    g_CallbackCount++;
    
    return E_OK;
}

/**
 * @brief Register RX indication
 */
Std_ReturnType CanIf_RegisterRxIndication(uint16 canId, 
                                           CanRxIndicationCallback callback) {
    if (g_CallbackCount >= CANIF_CALLBACK_COUNT || callback == NULL) {
        return E_NOT_OK;
    }
    
    g_Callbacks[g_CallbackCount].canId = canId;
    g_Callbacks[g_CallbackCount].rxCallback = callback;
    g_CallbackCount++;
    
    return E_OK;
}

/**
 * @brief Register error callback
 */
Std_ReturnType CanIf_RegisterErrorCallback(CanErrorCallback callback) {
    if (callback == NULL) {
        return E_NOT_OK;
    }
    
    g_ErrorCallback = callback;
    return E_OK;
}

/* =============================================================================
 * PERIODIC FUNCTIONS
 * ============================================================================= */

/**
 * @brief CanIf main function
 */
void CanIf_MainFunction(void) {
    if (g_CanIfStatus != CANIF_STARTED) {
        return;
    }
    
    CanIf_MainFunctionTx();
    CanIf_MainFunctionRx();
}

/**
 * @brief CanIf TX main function
 */
void CanIf_MainFunctionTx(void) {
    uint8 i;
    Std_ReturnType result;
    
    /* Process transmission queue */
    for (i = 0; i < g_TxQueueIndex; i++) {
        if (g_TxQueue[i].status == 0) {  /* Pending */
            /* Send via CAN driver */
            result = CanDrv_Transmit(&g_TxQueue[i].frame);
            
            if (result == E_OK) {
                g_TxQueue[i].status = 1;  /* Sent */
                g_Statistics.txCount++;
                
                /* Call TX indication callback */
                uint8 j;
                for (j = 0; j < g_CallbackCount; j++) {
                    if (g_Callbacks[j].canId == g_TxQueue[i].frame.canId &&
                        g_Callbacks[j].txCallback != NULL) {
                        g_Callbacks[j].txCallback(g_TxQueue[i].frame.canId);
                    }
                }
            }
        }
    }
}

/**
 * @brief CanIf RX main function
 */
void CanIf_MainFunctionRx(void) {
    CanFrameType frame;
    uint8 i;
    
    /* Receive from CAN driver */
    while (CanDrv_Receive(&frame) == E_OK) {
        /* Check filter */
        if (!CanIf_CheckFilter(frame.canId)) {
            continue;  /* Message filtered out */
        }
        
        /* Add to receive queue */
        if (g_RxQueueWrite < CANIF_RX_QUEUE_SIZE) {
            g_RxQueue[g_RxQueueWrite].frame = frame;
            g_RxQueue[g_RxQueueWrite].isNew = TRUE;
            g_RxQueue[g_RxQueueWrite].timestamp = 0;
            g_RxQueueWrite++;
            g_Statistics.rxCount++;
            
            /* Call RX indication callback */
            for (i = 0; i < g_CallbackCount; i++) {
                if (g_Callbacks[i].canId == frame.canId &&
                    g_Callbacks[i].rxCallback != NULL) {
                    g_Callbacks[i].rxCallback(frame.canId, frame.data, frame.dlc);
                }
            }
            
            /* Forward to COM layer */
            Com_ReceiveIPdu(CanIf_GetPduId(frame.canId), frame.data, frame.dlc);
        }
    }
}

/* =============================================================================
 * FRAME UTILITIES
 * ============================================================================= */

/**
 * @brief Create frame
 */
Std_ReturnType CanIf_CreateFrame(CanFrameType *frame, uint16 canId, 
                                  const uint8 *data, uint8 dlc) {
    if (frame == NULL || data == NULL || dlc > 8) {
        return E_NOT_OK;
    }
    
    frame->canId = canId;
    frame->dlc = dlc;
    frame->isExtended = FALSE;
    frame->isRemote = FALSE;
    memcpy(frame->data, data, dlc);
    
    return E_OK;
}

/**
 * @brief Extract frame
 */
Std_ReturnType CanIf_ExtractFrame(const CanFrameType *frame, uint16 *canId, 
                                   uint8 *data, uint8 *dlc) {
    if (frame == NULL || canId == NULL || data == NULL || dlc == NULL) {
        return E_NOT_OK;
    }
    
    *canId = frame->canId;
    *dlc = frame->dlc;
    memcpy(data, frame->data, frame->dlc);
    
    return E_OK;
}

/* =============================================================================
 * INTERNAL HELPER FUNCTIONS
 * ============================================================================= */

/**
 * @brief Check if CAN ID passes filters
 */
static boolean CanIf_CheckFilter(uint16 canId) {
    uint8 i;
    
    for (i = 0; i < g_FilterCount; i++) {
        if ((canId & g_RxFilters[i].mask) == (g_RxFilters[i].canId & g_RxFilters[i].mask)) {
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * @brief Get PDU ID from CAN ID
 */
static uint8 CanIf_GetPduId(uint16 canId) {
    switch (canId) {
        case 0x100:
            return COM_IPDU_DOOR_COMMAND;
        case 0x101:
            return COM_IPDU_DOOR_STATUS;
        case 0x7D0:
            return COM_IPDU_DIAGNOSTICS;
        default:
            return 0xFF;
    }
}

/* =============================================================================
 * END OF FILE
 * ============================================================================= */
