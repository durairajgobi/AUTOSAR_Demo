/**
 * @file Com.c
 * @brief COM Module Implementation
 * 
 * AUTOSAR Communication Layer - Handles all signal and PDU routing.
 * 
 * @version 1.0
 * @date 2024
 */

#include "Com.h"
#include "CanIf.h"
#include "Dem.h"
#include <string.h>

/* =============================================================================
 * INTERNAL DATA STRUCTURES
 * ============================================================================= */

/* COM Status */
typedef enum {
    COM_UNINIT = 0,
    COM_INIT = 1,
    COM_STARTED = 2
} ComStatusType;

static ComStatusType g_ComStatus = COM_UNINIT;

/* PDU Data Buffers */
static uint8 g_DoorCommandPdu[COM_PDU_DOOR_COMMAND_SIZE] = {0};
static uint8 g_DoorStatusPdu[COM_PDU_DOOR_STATUS_SIZE] = {0};
static uint8 g_DiagnosticsPdu[COM_PDU_DIAG_SIZE] = {0};

/* PDU Transmission Counters and Status */
typedef struct {
    uint32 txCounter;           /* Number of transmissions */
    uint32 rxCounter;           /* Number of receptions */
    uint32 lastTxTime;          /* Last transmission time */
    uint32 lastRxTime;          /* Last reception time */
    CanMessageStatusType status; /* PDU status */
    boolean isEnabled;          /* PDU enabled flag */
} ComPduStateType;

static ComPduStateType g_PduState[COM_PDU_COUNT] = {
    /* PDU 0: DoorCommand */
    {0, 0, 0, 0, CAN_MSG_NOT_SENT, TRUE},
    /* PDU 1: DoorStatus */
    {0, 0, 0, 0, CAN_MSG_NOT_SENT, TRUE},
    /* PDU 2: Diagnostics */
    {0, 0, 0, 0, CAN_MSG_NOT_SENT, TRUE}
};

/* Signal Transmission Counters */
typedef struct {
    uint32 txCounter;           /* Number of transmissions */
    uint32 rxCounter;           /* Number of receptions */
    boolean isNew;              /* New data flag */
    boolean isEnabled;          /* Signal enabled flag */
    uint8 data[8];              /* Signal data buffer */
    uint8 length;               /* Signal length */
    uint32 lastUpdateTime;      /* Last update time */
} ComSignalStateType;

static ComSignalStateType g_SignalState[COM_SIGNAL_COUNT];

/* PDU Configuration */
static const ComIPduConfigType g_IpduConfig[COM_PDU_COUNT] = {
    /* PDU 0: DoorCommand_PDU (0x100) */
    {
        .pduId = COM_IPDU_DOOR_COMMAND,
        .pduLength = COM_PDU_DOOR_COMMAND_SIZE,
        .canId = COM_PDU_DOOR_COMMAND,
        .transmissionMode = 0,  /* RX */
        .txPeriod = 0,
        .signalBasedTx = FALSE
    },
    /* PDU 1: DoorStatus_PDU (0x101) */
    {
        .pduId = COM_IPDU_DOOR_STATUS,
        .pduLength = COM_PDU_DOOR_STATUS_SIZE,
        .canId = COM_PDU_DOOR_STATUS,
        .transmissionMode = 1,  /* TX */
        .txPeriod = 10,         /* 10ms */
        .signalBasedTx = TRUE
    },
    /* PDU 2: Diagnostics_PDU (0x7D0) */
    {
        .pduId = COM_IPDU_DIAGNOSTICS,
        .pduLength = COM_PDU_DIAG_SIZE,
        .canId = COM_PDU_DIAGNOSTICS,
        .transmissionMode = 3,  /* TX/RX */
        .txPeriod = 50,         /* 50ms */
        .signalBasedTx = FALSE
    }
};

/* Signal Configuration */
static const ComISignalConfigType g_IsignalConfig[COM_SIGNAL_COUNT] = {
    /* Signal 0: DoorPosition (16 bits, PDU 1) */
    {0, 16, 0, 0, TRUE},
    /* Signal 1: DoorMotorCurrent (16 bits, PDU 1) */
    {1, 16, 16, 0, TRUE},
    /* Signal 2: DoorStatus (3 bits, PDU 1) */
    {2, 3, 32, 0, TRUE},
    /* Signal 3: DoorError (8 bits, PDU 1) */
    {3, 8, 35, 0, TRUE},
    /* Signal 4: DoorOpenCmd (1 bit, PDU 0) */
    {4, 1, 0, 0, TRUE},
    /* Signal 5: DoorCloseCmd (1 bit, PDU 0) */
    {5, 1, 1, 0, TRUE}
};

/* =============================================================================
 * INITIALIZATION
 * ============================================================================= */

/**
 * @brief Initialize COM module
 */
Std_ReturnType Com_Init(void) {
    uint8 i;
    
    if (g_ComStatus != COM_UNINIT) {
        return E_NOT_OK;
    }
    
    /* Initialize PDU state */
    for (i = 0; i < COM_PDU_COUNT; i++) {
        g_PduState[i].txCounter = 0;
        g_PduState[i].rxCounter = 0;
        g_PduState[i].status = CAN_MSG_NOT_SENT;
        g_PduState[i].isEnabled = TRUE;
    }
    
    /* Initialize signal state */
    for (i = 0; i < COM_SIGNAL_COUNT; i++) {
        g_SignalState[i].txCounter = 0;
        g_SignalState[i].rxCounter = 0;
        g_SignalState[i].isNew = FALSE;
        g_SignalState[i].isEnabled = TRUE;
        g_SignalState[i].length = 0;
        memset(g_SignalState[i].data, 0, sizeof(g_SignalState[i].data));
    }
    
    /* Initialize PDU buffers */
    memset(g_DoorCommandPdu, 0, COM_PDU_DOOR_COMMAND_SIZE);
    memset(g_DoorStatusPdu, 0, COM_PDU_DOOR_STATUS_SIZE);
    memset(g_DiagnosticsPdu, 0, COM_PDU_DIAG_SIZE);
    
    g_ComStatus = COM_INIT;
    return E_OK;
}

/**
 * @brief Deinitialize COM module
 */
Std_ReturnType Com_Deinit(void) {
    g_ComStatus = COM_UNINIT;
    return E_OK;
}

/**
 * @brief Start COM
 */
void Com_Start(void) {
    if (g_ComStatus == COM_INIT) {
        g_ComStatus = COM_STARTED;
    }
}

/**
 * @brief Stop COM
 */
void Com_Stop(void) {
    g_ComStatus = COM_INIT;
}

/* =============================================================================
 * SIGNAL TRANSMISSION
 * ============================================================================= */

/**
 * @brief Send signal data
 */
Std_ReturnType Com_SendSignal(uint8 signalId, const uint8 *data, uint8 length) {
    if (g_ComStatus != COM_STARTED) {
        return E_NOT_OK;
    }
    
    if (signalId >= COM_SIGNAL_COUNT || data == NULL) {
        return E_NOT_OK;
    }
    
    if (!g_SignalState[signalId].isEnabled) {
        return E_NOT_OK;
    }
    
    /* Copy signal data */
    if (length > sizeof(g_SignalState[signalId].data)) {
        length = sizeof(g_SignalState[signalId].data);
    }
    
    memcpy(g_SignalState[signalId].data, data, length);
    g_SignalState[signalId].length = length;
    g_SignalState[signalId].txCounter++;
    g_SignalState[signalId].lastUpdateTime = 0;  /* Would be current time in real impl */
    
    /* Pack signal into appropriate PDU */
    Com_PackSignalIntoPdu(signalId);
    
    /* Trigger PDU transmission if signal-based */
    if (g_IpduConfig[1].signalBasedTx) {  /* DoorStatus PDU */
        Com_TriggerIPduSend(COM_IPDU_DOOR_STATUS);
    }
    
    return E_OK;
}

/**
 * @brief Trigger PDU transmission
 */
Std_ReturnType Com_TriggerIPduSend(uint8 pduId) {
    Std_ReturnType result;
    
    if (pduId >= COM_PDU_COUNT) {
        return E_NOT_OK;
    }
    
    if (!g_PduState[pduId].isEnabled) {
        return E_NOT_OK;
    }
    
    /* Get PDU data based on ID */
    uint8 *pduData = NULL;
    uint8 pduLength = 0;
    
    switch (pduId) {
        case COM_IPDU_DOOR_STATUS:
            pduData = g_DoorStatusPdu;
            pduLength = COM_PDU_DOOR_STATUS_SIZE;
            break;
        case COM_IPDU_DIAGNOSTICS:
            pduData = g_DiagnosticsPdu;
            pduLength = COM_PDU_DIAG_SIZE;
            break;
        default:
            return E_NOT_OK;
    }
    
    /* Send via CAN interface */
    result = CanIf_Transmit(g_IpduConfig[pduId].canId, pduData, pduLength);
    
    if (result == E_OK) {
        g_PduState[pduId].txCounter++;
        g_PduState[pduId].status = CAN_MSG_SENT;
    } else {
        g_PduState[pduId].status = CAN_MSG_ERROR;
    }
    
    return result;
}

/**
 * @brief Enable signal transmission
 */
Std_ReturnType Com_EnableSignalTransmission(uint8 signalId, boolean enable) {
    if (signalId >= COM_SIGNAL_COUNT) {
        return E_NOT_OK;
    }
    
    g_SignalState[signalId].isEnabled = enable ? TRUE : FALSE;
    return E_OK;
}

/* =============================================================================
 * PDU TRANSMISSION
 * ============================================================================= */

/**
 * @brief Send I-PDU
 */
Std_ReturnType Com_SendIPdu(uint8 pduId, const uint8 *pduData, uint8 pduLength) {
    Std_ReturnType result;
    uint8 *buffer = NULL;
    uint8 *length = NULL;
    
    if (pduId >= COM_PDU_COUNT || pduData == NULL) {
        return E_NOT_OK;
    }
    
    /* Get PDU buffer */
    switch (pduId) {
        case COM_IPDU_DOOR_COMMAND:
            buffer = g_DoorCommandPdu;
            length = (uint8 *)&COM_PDU_DOOR_COMMAND_SIZE;
            break;
        case COM_IPDU_DOOR_STATUS:
            buffer = g_DoorStatusPdu;
            length = (uint8 *)&COM_PDU_DOOR_STATUS_SIZE;
            break;
        case COM_IPDU_DIAGNOSTICS:
            buffer = g_DiagnosticsPdu;
            length = (uint8 *)&COM_PDU_DIAG_SIZE;
            break;
        default:
            return E_NOT_OK;
    }
    
    /* Copy PDU data */
    memcpy(buffer, pduData, pduLength);
    
    /* Send via CAN */
    result = CanIf_Transmit(g_IpduConfig[pduId].canId, buffer, pduLength);
    
    if (result == E_OK) {
        g_PduState[pduId].txCounter++;
        g_PduState[pduId].status = CAN_MSG_SENT;
    } else {
        g_PduState[pduId].status = CAN_MSG_ERROR;
    }
    
    return result;
}

/**
 * @brief Get transmission status
 */
CanMessageStatusType Com_GetIPduTransmitStatus(uint8 pduId) {
    if (pduId >= COM_PDU_COUNT) {
        return CAN_MSG_ERROR;
    }
    return g_PduState[pduId].status;
}

/* =============================================================================
 * SIGNAL RECEPTION
 * ============================================================================= */

/**
 * @brief Receive signal data
 */
Std_ReturnType Com_ReceiveSignal(uint8 signalId, uint8 *data, uint8 *length) {
    if (signalId >= COM_SIGNAL_COUNT || data == NULL || length == NULL) {
        return E_NOT_OK;
    }
    
    if (g_SignalState[signalId].length == 0) {
        return E_NOT_OK;
    }
    
    memcpy(data, g_SignalState[signalId].data, g_SignalState[signalId].length);
    *length = g_SignalState[signalId].length;
    
    return E_OK;
}

/**
 * @brief Enable signal reception
 */
Std_ReturnType Com_EnableSignalReception(uint8 signalId, boolean enable) {
    if (signalId >= COM_SIGNAL_COUNT) {
        return E_NOT_OK;
    }
    
    g_SignalState[signalId].isEnabled = enable ? TRUE : FALSE;
    return E_OK;
}

/* =============================================================================
 * PDU RECEPTION
 * ============================================================================= */

/**
 * @brief Receive I-PDU
 */
Std_ReturnType Com_ReceiveIPdu(uint8 pduId, const uint8 *pduData, uint8 pduLength) {
    uint8 *buffer = NULL;
    
    if (pduId >= COM_PDU_COUNT || pduData == NULL) {
        return E_NOT_OK;
    }
    
    /* Get PDU buffer */
    switch (pduId) {
        case COM_IPDU_DOOR_COMMAND:
            buffer = g_DoorCommandPdu;
            break;
        case COM_IPDU_DIAGNOSTICS:
            buffer = g_DiagnosticsPdu;
            break;
        default:
            return E_NOT_OK;
    }
    
    /* Copy received PDU */
    memcpy(buffer, pduData, pduLength);
    
    /* Update PDU state */
    g_PduState[pduId].rxCounter++;
    g_PduState[pduId].status = CAN_MSG_RECEIVED;
    
    /* Unpack signals from PDU */
    Com_UnpackSignalsFromPdu(pduId);
    
    return E_OK;
}

/**
 * @brief Get reception status
 */
CanMessageStatusType Com_GetIPduReceiveStatus(uint8 pduId) {
    if (pduId >= COM_PDU_COUNT) {
        return CAN_MSG_ERROR;
    }
    return g_PduState[pduId].status;
}

/**
 * @brief Check if signal is new
 */
boolean Com_IsSignalNew(uint8 signalId) {
    if (signalId >= COM_SIGNAL_COUNT) {
        return FALSE;
    }
    
    boolean isNew = g_SignalState[signalId].isNew;
    g_SignalState[signalId].isNew = FALSE;  /* Clear flag */
    
    return isNew;
}

/* =============================================================================
 * PERIODIC PROCESSING
 * ============================================================================= */

/**
 * @brief COM main function
 */
void Com_MainFunction(void) {
    if (g_ComStatus != COM_STARTED) {
        return;
    }
    
    Com_MainFunctionTx();
    Com_MainFunctionRx();
}

/**
 * @brief COM transmission main function
 */
void Com_MainFunctionTx(void) {
    /* Handle timed PDU transmissions */
    static uint32 lastTxTime = 0;
    uint32 currentTime = 0;  /* Would be actual time in real impl */
    
    /* Transmit DoorStatus every 10ms */
    if (currentTime - lastTxTime >= 10) {
        Com_TriggerIPduSend(COM_IPDU_DOOR_STATUS);
        lastTxTime = currentTime;
    }
}

/**
 * @brief COM reception main function
 */
void Com_MainFunctionRx(void) {
    /* Handle reception timeout monitoring */
    uint8 i;
    
    for (i = 0; i < COM_PDU_COUNT; i++) {
        if (g_IpduConfig[i].transmissionMode == 0) {  /* RX */
            /* Check timeout */
        }
    }
}

/* =============================================================================
 * STATUS AND MONITORING
 * ============================================================================= */

/**
 * @brief Get COM status
 */
uint8 Com_GetStatus(void) {
    return (uint8)g_ComStatus;
}

/**
 * @brief Get signal status
 */
uint8 Com_GetSignalStatus(uint8 signalId) {
    if (signalId >= COM_SIGNAL_COUNT) {
        return 0;
    }
    return g_SignalState[signalId].isEnabled ? 1 : 0;
}

/**
 * @brief Get PDU counter
 */
uint32 Com_GetPduCounter(uint8 pduId) {
    if (pduId >= COM_PDU_COUNT) {
        return 0;
    }
    return g_PduState[pduId].txCounter + g_PduState[pduId].rxCounter;
}

/* =============================================================================
 * CONFIGURATION
 * ============================================================================= */

/**
 * @brief Get PDU configuration
 */
Std_ReturnType Com_GetIPduConfig(uint8 pduId, ComIPduConfigType *config) {
    if (pduId >= COM_PDU_COUNT || config == NULL) {
        return E_NOT_OK;
    }
    
    *config = g_IpduConfig[pduId];
    return E_OK;
}

/**
 * @brief Get signal configuration
 */
Std_ReturnType Com_GetISignalConfig(uint8 signalId, ComISignalConfigType *config) {
    if (signalId >= COM_SIGNAL_COUNT || config == NULL) {
        return E_NOT_OK;
    }
    
    *config = g_IsignalConfig[signalId];
    return E_OK;
}

/* =============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================= */

/**
 * @brief Pack signal into PDU
 */
static void Com_PackSignalIntoPdu(uint8 signalId) {
    /* Convert signal data format and pack into PDU buffer */
    uint8 *pdu = NULL;
    const ComISignalConfigType *config = &g_IsignalConfig[signalId];
    
    /* Determine target PDU */
    if (signalId < 4) {  /* DoorStatus PDU */
        pdu = g_DoorStatusPdu;
    }
    
    if (pdu != NULL) {
        /* Pack signal data into PDU at specified bit position */
        /* This is a simplified implementation */
        if (config->signalLength == 16) {
            uint16 data = *(uint16 *)g_SignalState[signalId].data;
            pdu[config->startBit / 8] = (uint8)(data & 0xFF);
            if (config->startBit / 8 + 1 < COM_PDU_DOOR_STATUS_SIZE) {
                pdu[config->startBit / 8 + 1] = (uint8)((data >> 8) & 0xFF);
            }
        }
    }
}

/**
 * @brief Unpack signals from PDU
 */
static void Com_UnpackSignalsFromPdu(uint8 pduId) {
    uint8 *pdu = NULL;
    uint8 i;
    
    /* Get PDU buffer */
    switch (pduId) {
        case COM_IPDU_DOOR_COMMAND:
            pdu = g_DoorCommandPdu;
            break;
        case COM_IPDU_DIAGNOSTICS:
            pdu = g_DiagnosticsPdu;
            break;
        default:
            return;
    }
    
    /* Unpack signals */
    for (i = 0; i < COM_SIGNAL_COUNT; i++) {
        const ComISignalConfigType *config = &g_IsignalConfig[i];
        
        if (config->startBit >= 0) {
            /* Extract signal from PDU and store in signal buffer */
            g_SignalState[i].isNew = TRUE;
            g_SignalState[i].rxCounter++;
        }
    }
}

/* =============================================================================
 * ERROR HANDLING
 * ============================================================================= */

/**
 * @brief Handle transmission error
 */
void Com_OnTransmissionError(uint8 pduId) {
    if (pduId < COM_PDU_COUNT) {
        g_PduState[pduId].status = CAN_MSG_ERROR;
        Dem_SetEventStatus(0xC10005, TRUE);  /* Report error to DEM */
    }
}

/**
 * @brief Handle reception error
 */
void Com_OnReceptionError(uint8 pduId) {
    if (pduId < COM_PDU_COUNT) {
        g_PduState[pduId].status = CAN_MSG_ERROR;
    }
}

/**
 * @brief Handle timeout error
 */
void Com_OnTimeoutError(uint8 pduId) {
    if (pduId < COM_PDU_COUNT) {
        g_PduState[pduId].status = CAN_MSG_TIMEOUT;
    }
}

/* =============================================================================
 * END OF FILE
 * ============================================================================= */
