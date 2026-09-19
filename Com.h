/**
 * @file Com.h
 * @brief COM Module - Communication Driver
 * 
 * Implements AUTOSAR Communication (COM) layer.
 * Handles PDU transmission, reception, and signal routing.
 * 
 * AUTOSAR 4.3.1 Compliant
 * 
 * @version 1.0
 * @date 2024
 */

#ifndef COM_H
#define COM_H

#include "Rte_Type.h"

/* =============================================================================
 * PDU DEFINITIONS
 * ============================================================================= */

#define COM_PDU_DOOR_COMMAND    0x100  /* CAN ID 0x100 (RX) */
#define COM_PDU_DOOR_STATUS     0x101  /* CAN ID 0x101 (TX) */
#define COM_PDU_DIAGNOSTICS     0x7D0  /* CAN ID 0x7D0 (RX/TX) */

#define COM_PDU_COUNT           3

/* PDU Sizes */
#define COM_PDU_DOOR_COMMAND_SIZE   2  /* Bytes */
#define COM_PDU_DOOR_STATUS_SIZE    6  /* Bytes */
#define COM_PDU_DIAG_SIZE           8  /* Bytes */

/* =============================================================================
 * SIGNAL DEFINITIONS
 * ============================================================================= */

/* Signal IDs for DoorStatus_PDU (0x101) */
#define COM_SIGNAL_DOOR_POSITION        0
#define COM_SIGNAL_DOOR_MOTOR_CURRENT   1
#define COM_SIGNAL_DOOR_STATUS          2
#define COM_SIGNAL_DOOR_ERROR           3

/* Signal IDs for DoorCommand_PDU (0x100) */
#define COM_SIGNAL_DOOR_OPEN_CMD        4
#define COM_SIGNAL_DOOR_CLOSE_CMD       5

#define COM_SIGNAL_COUNT                6

/* =============================================================================
 * I-PDU TYPES
 * ============================================================================= */

typedef enum {
    COM_IPDU_DOOR_COMMAND = 0,
    COM_IPDU_DOOR_STATUS = 1,
    COM_IPDU_DIAGNOSTICS = 2
} ComIPduType;

/* I-PDU Configuration */
typedef struct {
    uint16 pduId;               /* PDU identifier */
    uint8 pduLength;            /* PDU length in bytes */
    uint16 canId;               /* CAN message ID */
    uint8 transmissionMode;     /* TX/RX */
    uint32 txPeriod;            /* Transmission period (ms) */
    boolean signalBasedTx;      /* Trigger TX on signal change */
} ComIPduConfigType;

/* =============================================================================
 * I-SIGNAL TYPES
 * ============================================================================= */

typedef struct {
    uint8 signalId;
    uint8 signalLength;         /* Length in bits */
    uint8 startBit;             /* Start bit in PDU */
    uint8 byteOrder;            /* 0=Intel, 1=Motorola */
    boolean isSignal;           /* true=signal, false=padding */
} ComISignalConfigType;

/* =============================================================================
 * INITIALIZATION
 * ============================================================================= */

/**
 * @brief Initialize COM module
 * 
 * Initializes all PDUs, signals, and communication handlers.
 * 
 * @return E_OK if successful
 */
Std_ReturnType Com_Init(void);

/**
 * @brief Deinitialize COM module
 * 
 * @return E_OK if successful
 */
Std_ReturnType Com_Deinit(void);

/**
 * @brief Start communication
 * 
 * Enables all communication groups.
 */
void Com_Start(void);

/**
 * @brief Stop communication
 * 
 * Disables all communication groups.
 */
void Com_Stop(void);

/* =============================================================================
 * SIGNAL TRANSMISSION
 * ============================================================================= */

/**
 * @brief Send signal data
 * 
 * Sends signal data via COM layer. Triggers PDU transmission if needed.
 * 
 * @param[in] signalId: Signal identifier
 * @param[in] data: Pointer to signal data
 * @param[in] length: Data length in bytes
 * @return E_OK if successful
 */
Std_ReturnType Com_SendSignal(uint8 signalId, const uint8 *data, uint8 length);

/**
 * @brief Trigger PDU transmission
 * 
 * Immediately transmits a PDU.
 * 
 * @param[in] pduId: PDU identifier (COM_IPDU_*)
 * @return E_OK if successful
 */
Std_ReturnType Com_TriggerIPduSend(uint8 pduId);

/**
 * @brief Enable/Disable signal transmission
 * 
 * @param[in] signalId: Signal identifier
 * @param[in] enable: true=enable, false=disable
 * @return E_OK if successful
 */
Std_ReturnType Com_EnableSignalTransmission(uint8 signalId, boolean enable);

/* =============================================================================
 * PDU TRANSMISSION
 * ============================================================================= */

/**
 * @brief Send I-PDU
 * 
 * Transmits a complete I-PDU.
 * 
 * @param[in] pduId: PDU identifier
 * @param[in] pduData: Pointer to PDU data
 * @param[in] pduLength: PDU length
 * @return E_OK if queued for transmission
 */
Std_ReturnType Com_SendIPdu(uint8 pduId, const uint8 *pduData, uint8 pduLength);

/**
 * @brief Get transmission status
 * 
 * @param[in] pduId: PDU identifier
 * @return Status of PDU transmission
 */
CanMessageStatusType Com_GetIPduTransmitStatus(uint8 pduId);

/* =============================================================================
 * SIGNAL RECEPTION
 * ============================================================================= */

/**
 * @brief Receive signal data
 * 
 * Receives signal data from RTE/HAL.
 * 
 * @param[in] signalId: Signal identifier
 * @param[out] data: Pointer to receive buffer
 * @param[out] length: Received data length
 * @return E_OK if data available
 */
Std_ReturnType Com_ReceiveSignal(uint8 signalId, uint8 *data, uint8 *length);

/**
 * @brief Enable/disable signal reception
 * 
 * @param[in] signalId: Signal identifier
 * @param[in] enable: true=enable, false=disable
 * @return E_OK if successful
 */
Std_ReturnType Com_EnableSignalReception(uint8 signalId, boolean enable);

/* =============================================================================
 * PDU RECEPTION
 * ============================================================================= */

/**
 * @brief Receive I-PDU
 * 
 * Receives and processes a PDU from CAN layer.
 * 
 * @param[in] pduId: PDU identifier
 * @param[in] pduData: Pointer to PDU data
 * @param[in] pduLength: PDU length
 * @return E_OK if processed successfully
 */
Std_ReturnType Com_ReceiveIPdu(uint8 pduId, const uint8 *pduData, uint8 pduLength);

/**
 * @brief Get reception status
 * 
 * @param[in] pduId: PDU identifier
 * @return Status of PDU reception
 */
CanMessageStatusType Com_GetIPduReceiveStatus(uint8 pduId);

/**
 * @brief Check if signal data is new
 * 
 * @param[in] signalId: Signal identifier
 * @return true if new data available
 */
boolean Com_IsSignalNew(uint8 signalId);

/* =============================================================================
 * PERIODIC PROCESSING
 * ============================================================================= */

/**
 * @brief COM main function
 * 
 * Called periodically (typically 10-50ms) to handle:
 * - Timed signal transmission
 * - Deadline monitoring
 * - Signal updates
 * 
 * Period: 10ms (configurable)
 */
void Com_MainFunction(void);

/**
 * @brief COM transmission main function
 * 
 * Handles transmission of timed signals.
 */
void Com_MainFunctionTx(void);

/**
 * @brief COM reception main function
 * 
 * Handles reception of timed signals.
 */
void Com_MainFunctionRx(void);

/* =============================================================================
 * STATUS AND MONITORING
 * ============================================================================= */

/**
 * @brief Get COM status
 * 
 * @return COM module status
 */
uint8 Com_GetStatus(void);

/**
 * @brief Get signal status
 * 
 * @param[in] signalId: Signal identifier
 * @return Signal transmission/reception status
 */
uint8 Com_GetSignalStatus(uint8 signalId);

/**
 * @brief Get PDU counter
 * 
 * @param[in] pduId: PDU identifier
 * @return Number of times PDU was transmitted/received
 */
uint32 Com_GetPduCounter(uint8 pduId);

/* =============================================================================
 * CONFIGURATION
 * ============================================================================= */

/**
 * @brief Get PDU configuration
 * 
 * @param[in] pduId: PDU identifier
 * @param[out] config: Pointer to configuration structure
 * @return E_OK if configuration found
 */
Std_ReturnType Com_GetIPduConfig(uint8 pduId, ComIPduConfigType *config);

/**
 * @brief Get signal configuration
 * 
 * @param[in] signalId: Signal identifier
 * @param[out] config: Pointer to configuration structure
 * @return E_OK if configuration found
 */
Std_ReturnType Com_GetISignalConfig(uint8 signalId, ComISignalConfigType *config);

/* =============================================================================
 * ERROR HANDLING
 * ============================================================================= */

/**
 * @brief Handle transmission error
 * 
 * @param[in] pduId: PDU identifier
 */
void Com_OnTransmissionError(uint8 pduId);

/**
 * @brief Handle reception error
 * 
 * @param[in] pduId: PDU identifier
 */
void Com_OnReceptionError(uint8 pduId);

/**
 * @brief Handle timeout error
 * 
 * @param[in] pduId: PDU identifier
 */
void Com_OnTimeoutError(uint8 pduId);

/* =============================================================================
 * END OF FILE
 * ============================================================================= */

#endif /* COM_H */
