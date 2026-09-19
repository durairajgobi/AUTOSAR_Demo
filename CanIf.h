/**
 * @file CanIf.h
 * @brief CAN Interface Module
 * 
 * AUTOSAR CanIf Layer - Bridges COM layer and CAN driver.
 * Handles CAN message TX/RX, routing, and filtering.
 * 
 * AUTOSAR 4.3.1 Compliant
 * 
 * @version 1.0
 * @date 2024
 */

#ifndef CANIF_H
#define CANIF_H

#include "Rte_Type.h"

/* =============================================================================
 * CAN MESSAGE TYPES
 * ============================================================================= */

/* CAN Frame */
typedef struct {
    uint16 canId;               /* CAN message ID */
    uint8 dlc;                  /* Data length code (0-8) */
    uint8 data[8];              /* CAN data bytes */
    boolean isExtended;         /* Extended ID flag */
    boolean isRemote;           /* Remote request flag */
} CanFrameType;

/* Controller state */
typedef enum {
    CAN_CTRL_UNINIT = 0,
    CAN_CTRL_STOPPED = 1,
    CAN_CTRL_STARTED = 2,
    CAN_CTRL_ERROR = 3
} CanControllerStateType;

/* =============================================================================
 * INITIALIZATION
 * ============================================================================= */

/**
 * @brief Initialize CanIf module
 * 
 * Initializes all CAN controllers and message filters.
 * 
 * @return E_OK if successful
 */
Std_ReturnType CanIf_Init(void);

/**
 * @brief Shutdown CanIf module
 * 
 * @return E_OK if successful
 */
Std_ReturnType CanIf_Shutdown(void);

/**
 * @brief Start CAN controller
 * 
 * Starts CAN communication on controller 0.
 * 
 * @return E_OK if successful
 */
Std_ReturnType CanIf_Start(void);

/**
 * @brief Stop CAN controller
 * 
 * Stops CAN communication.
 * 
 * @return E_OK if successful
 */
Std_ReturnType CanIf_Stop(void);

/* =============================================================================
 * TRANSMISSION
 * ============================================================================= */

/**
 * @brief Transmit CAN message
 * 
 * Queues a CAN message for transmission.
 * 
 * @param[in] canId: CAN message ID (11-bit or 29-bit)
 * @param[in] data: Pointer to data buffer (0-8 bytes)
 * @param[in] dlc: Data length code
 * @return E_OK if queued successfully
 */
Std_ReturnType CanIf_Transmit(uint16 canId, const uint8 *data, uint8 dlc);

/**
 * @brief Transmit CAN frame (full control)
 * 
 * @param[in] frame: Pointer to CAN frame structure
 * @return E_OK if transmitted
 */
Std_ReturnType CanIf_TransmitFrame(const CanFrameType *frame);

/**
 * @brief Get transmission status
 * 
 * @param[in] canId: CAN message ID
 * @return Status (0=idle, 1=pending, 2=success, 3=error)
 */
uint8 CanIf_GetTransmitStatus(uint16 canId);

/**
 * @brief Cancel transmission
 * 
 * @param[in] canId: CAN message ID
 * @return E_OK if cancelled
 */
Std_ReturnType CanIf_CancelTransmit(uint16 canId);

/* =============================================================================
 * RECEPTION
 * ============================================================================= */

/**
 * @brief Receive CAN message
 * 
 * Receives queued CAN messages.
 * 
 * @param[in] canId: Expected CAN message ID
 * @param[out] data: Buffer for received data
 * @param[out] dlc: Pointer to receive data length
 * @return E_OK if message received
 */
Std_ReturnType CanIf_Receive(uint16 canId, uint8 *data, uint8 *dlc);

/**
 * @brief Receive CAN frame (full)
 * 
 * @param[out] frame: Pointer to receive frame
 * @return E_OK if frame received
 */
Std_ReturnType CanIf_ReceiveFrame(CanFrameType *frame);

/**
 * @brief Get reception status
 * 
 * @param[in] canId: CAN message ID
 * @return Status (0=no message, 1=message ready, 2=timeout)
 */
uint8 CanIf_GetReceiveStatus(uint16 canId);

/**
 * @brief Check if message available
 * 
 * @param[in] canId: CAN message ID
 * @return true if message available
 */
boolean CanIf_IsMessageAvailable(uint16 canId);

/* =============================================================================
 * MESSAGE FILTERING
 * ============================================================================= */

/**
 * @brief Add receive filter
 * 
 * @param[in] canId: CAN message ID
 * @param[in] mask: ID mask (0xFFFF for exact match)
 * @return E_OK if successful
 */
Std_ReturnType CanIf_AddReceiveFilter(uint16 canId, uint16 mask);

/**
 * @brief Remove receive filter
 * 
 * @param[in] canId: CAN message ID
 * @return E_OK if successful
 */
Std_ReturnType CanIf_RemoveReceiveFilter(uint16 canId);

/**
 * @brief Clear all receive filters
 * 
 * @return E_OK if successful
 */
Std_ReturnType CanIf_ClearAllFilters(void);

/**
 * @brief Get filter count
 * 
 * @return Number of active filters
 */
uint8 CanIf_GetFilterCount(void);

/* =============================================================================
 * CONFIGURATION
 * ============================================================================= */

/**
 * @brief Set CAN baud rate
 * 
 * @param[in] baudRate: Baud rate in kbps (e.g., 500 for 500kbps)
 * @return E_OK if successful
 */
Std_ReturnType CanIf_SetBaudRate(uint16 baudRate);

/**
 * @brief Get CAN baud rate
 * 
 * @return Current baud rate in kbps
 */
uint16 CanIf_GetBaudRate(void);

/**
 * @brief Set controller mode
 * 
 * @param[in] mode: 0=stopped, 1=normal, 2=listen-only
 * @return E_OK if successful
 */
Std_ReturnType CanIf_SetControllerMode(uint8 mode);

/* =============================================================================
 * CONTROLLER MANAGEMENT
 * ============================================================================= */

/**
 * @brief Get controller state
 * 
 * @return Controller state
 */
CanControllerStateType CanIf_GetControllerState(void);

/**
 * @brief Disable controller
 * 
 * @return E_OK if successful
 */
Std_ReturnType CanIf_DisableController(void);

/**
 * @brief Enable controller
 * 
 * @return E_OK if successful
 */
Std_ReturnType CanIf_EnableController(void);

/**
 * @brief Reset controller
 * 
 * Performs software reset of CAN controller.
 * 
 * @return E_OK if successful
 */
Std_ReturnType CanIf_ResetController(void);

/**
 * @brief Perform wake-up
 * 
 * @return E_OK if successful
 */
Std_ReturnType CanIf_Wakeup(void);

/**
 * @brief Set sleep mode
 * 
 * @return E_OK if successful
 */
Std_ReturnType CanIf_GoToSleep(void);

/* =============================================================================
 * STATUS AND MONITORING
 * ============================================================================= */

/**
 * @brief Get CanIf status
 * 
 * @return Module status (0=uninit, 1=init, 2=started)
 */
uint8 CanIf_GetStatus(void);

/**
 * @brief Get bus statistics
 * 
 * @param[out] txCount: Number of transmitted messages
 * @param[out] rxCount: Number of received messages
 * @param[out] errorCount: Number of errors
 * @return E_OK if successful
 */
Std_ReturnType CanIf_GetBusStatistics(uint32 *txCount, uint32 *rxCount, 
                                       uint32 *errorCount);

/**
 * @brief Get error counter
 * 
 * @return CAN bus error counter
 */
uint16 CanIf_GetErrorCounter(void);

/**
 * @brief Check bus-off status
 * 
 * @return true if bus-off, false otherwise
 */
boolean CanIf_IsBusOff(void);

/**
 * @brief Check error frame received
 * 
 * @return true if error frame detected
 */
boolean CanIf_IsErrorFrameReceived(void);

/* =============================================================================
 * ROUTING AND CALLBACKS
 * ============================================================================= */

/**
 * @brief Register TX indication callback
 * 
 * @param[in] canId: CAN message ID
 * @param[in] callback: Callback function pointer
 * @return E_OK if registered
 */
typedef void (*CanTxIndicationCallback)(uint16 canId);
Std_ReturnType CanIf_RegisterTxIndication(uint16 canId, 
                                           CanTxIndicationCallback callback);

/**
 * @brief Register RX indication callback
 * 
 * @param[in] canId: CAN message ID
 * @param[in] callback: Callback function pointer
 * @return E_OK if registered
 */
typedef void (*CanRxIndicationCallback)(uint16 canId, const uint8 *data, 
                                         uint8 dlc);
Std_ReturnType CanIf_RegisterRxIndication(uint16 canId, 
                                           CanRxIndicationCallback callback);

/**
 * @brief Register error callback
 * 
 * @param[in] callback: Callback function pointer
 * @return E_OK if registered
 */
typedef void (*CanErrorCallback)(uint8 errorType);
Std_ReturnType CanIf_RegisterErrorCallback(CanErrorCallback callback);

/* =============================================================================
 * PERIODIC FUNCTIONS
 * ============================================================================= */

/**
 * @brief CanIf main function
 * 
 * Handles CAN controller polling and callback execution.
 * Called periodically (10-50ms).
 */
void CanIf_MainFunction(void);

/**
 * @brief CanIf transmission main function
 */
void CanIf_MainFunctionTx(void);

/**
 * @brief CanIf reception main function
 */
void CanIf_MainFunctionRx(void);

/* =============================================================================
 * FRAME UTILITIES
 * ============================================================================= */

/**
 * @brief Create CAN frame
 * 
 * Utility to create a CAN frame from components.
 * 
 * @param[out] frame: Pointer to frame structure
 * @param[in] canId: CAN message ID
 * @param[in] data: Pointer to data
 * @param[in] dlc: Data length
 * @return E_OK if successful
 */
Std_ReturnType CanIf_CreateFrame(CanFrameType *frame, uint16 canId, 
                                  const uint8 *data, uint8 dlc);

/**
 * @brief Extract frame data
 * 
 * @param[in] frame: Pointer to frame
 * @param[out] canId: Pointer to receive CAN ID
 * @param[out] data: Pointer to data buffer
 * @param[out] dlc: Pointer to receive DLC
 * @return E_OK if successful
 */
Std_ReturnType CanIf_ExtractFrame(const CanFrameType *frame, uint16 *canId, 
                                   uint8 *data, uint8 *dlc);

/* =============================================================================
 * END OF FILE
 * ============================================================================= */

#endif /* CANIF_H */
