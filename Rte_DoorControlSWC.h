/**
 * @file Rte_DoorControlSWC.h
 * @brief RTE Interface for DoorControlSWC
 * 
 * Provides port abstractions and data access functions for the Door Control SWC.
 * AUTOSAR 4.3.1 Compliant
 * 
 * Generated from: 01_SystemDescription.arxml
 * 
 * @version 1.0
 * @date 2024
 */

#ifndef RTE_DOORCONTROLSWC_H
#define RTE_DOORCONTROLSWC_H

#include "Rte_Type.h"

/* =============================================================================
 * SENDER-RECEIVER PORT: DoorStatus_Out
 * ============================================================================= */

/**
 * @brief Write door status data to output port
 * 
 * Sends door status to the CAN network via COM layer.
 * 
 * @param[in] status: Pointer to DoorStatusType structure
 * @return E_OK if successful
 */
Std_ReturnType Rte_Write_DoorStatus_Out_DoorStatus(
    const DoorStatusType *status);

/**
 * @brief Write door position
 * 
 * @param[in] position: Door position (0-100%)
 * @return E_OK if successful
 */
Std_ReturnType Rte_Write_DoorStatus_Out_Position(uint16 position);

/**
 * @brief Write motor current
 * 
 * @param[in] current: Motor current in mA
 * @return E_OK if successful
 */
Std_ReturnType Rte_Write_DoorStatus_Out_MotorCurrent(uint16 current);

/**
 * @brief Write door state
 * 
 * @param[in] state: Door state
 * @return E_OK if successful
 */
Std_ReturnType Rte_Write_DoorStatus_Out_State(DoorStateType state);

/**
 * @brief Write door error
 * 
 * @param[in] error: Error code
 * @return E_OK if successful
 */
Std_ReturnType Rte_Write_DoorStatus_Out_Error(DoorErrorCodeType error);

/* =============================================================================
 * RECEIVER PORT: DoorCommand_In
 * ============================================================================= */

/**
 * @brief Read door command from input port
 * 
 * Receives door open/close commands from CAN network via COM layer.
 * 
 * @param[out] command: Pointer to DoorCommandType structure
 * @return E_OK if data is available and valid
 */
Std_ReturnType Rte_Read_DoorCommand_In_DoorCommand(
    DoorCommandType *command);

/**
 * @brief Read door open command
 * 
 * @param[out] openCmd: Pointer to open command flag
 * @return E_OK if successful
 */
Std_ReturnType Rte_Read_DoorCommand_In_OpenCmd(boolean *openCmd);

/**
 * @brief Read door close command
 * 
 * @param[out] closeCmd: Pointer to close command flag
 * @return E_OK if successful
 */
Std_ReturnType Rte_Read_DoorCommand_In_CloseCmd(boolean *closeCmd);

/* =============================================================================
 * RECEIVER PORT: DoorSensor_In
 * ============================================================================= */

/**
 * @brief Read sensor data from input port
 * 
 * Receives position and current sensor readings.
 * 
 * @param[out] sensor: Pointer to SensorDataType structure
 * @return E_OK if data is valid
 */
Std_ReturnType Rte_Read_DoorSensor_In_SensorData(
    SensorDataType *sensor);

/**
 * @brief Read position sensor
 * 
 * @param[out] position: Pointer to position value (0-100%)
 * @return E_OK if successful
 */
Std_ReturnType Rte_Read_DoorSensor_In_Position(uint16 *position);

/**
 * @brief Read current sensor
 * 
 * @param[out] current: Pointer to current value (mA)
 * @return E_OK if successful
 */
Std_ReturnType Rte_Read_DoorSensor_In_Current(uint16 *current);

/* =============================================================================
 * CLIENT-SERVER PORT: MotorControl_Out
 * ============================================================================= */

/**
 * @brief Control motor speed and direction
 * 
 * Invokes motor control service.
 * 
 * @param[in] speed: Motor speed (-100 to +100)
 * @param[in] direction: Motor direction (STOP/OPEN/CLOSE)
 * @return E_OK if service executed successfully
 */
Std_ReturnType Rte_Call_MotorControl_Out_SetMotorSpeed(
    sint8 speed,
    MotorDirectionType direction);

/**
 * @brief Stop motor
 * 
 * @return E_OK if successful
 */
Std_ReturnType Rte_Call_MotorControl_Out_StopMotor(void);

/**
 * @brief Enable/disable motor
 * 
 * @param[in] enable: true=enable, false=disable
 * @return E_OK if successful
 */
Std_ReturnType Rte_Call_MotorControl_Out_EnableMotor(boolean enable);

/* =============================================================================
 * TIMING EVENT INTERFACE
 * ============================================================================= */

/**
 * @brief Get current system time
 * 
 * @return Current time in milliseconds
 */
uint32 Rte_GetTimestamp(void);

/**
 * @brief Check if timing event is due
 * 
 * @param[in] lastTime: Last execution time
 * @param[in] period: Period in milliseconds
 * @return true if period elapsed
 */
boolean Rte_CheckTimingEvent(uint32 lastTime, uint32 period);

/* =============================================================================
 * RUNNABLE ENTRY POINT
 * ============================================================================= */

/**
 * @brief DoorControl_Runnable
 * 
 * Main control logic runnable, called every 10ms by RTE.
 * Period: 10 ms
 */
void DoorControl_Runnable_10ms(void);

/* =============================================================================
 * INTERRUPT/ERROR HANDLING
 * ============================================================================= */

/**
 * @brief RTE Error callback
 * 
 * Called when an RTE error occurs.
 * 
 * @param[in] errorCode: Error code
 */
void Rte_ErrorHandler(uint8 errorCode);

/**
 * @brief Check port validity
 * 
 * Verifies if received data is valid and not timed out.
 * 
 * @return true if all ports are valid
 */
boolean Rte_IsDataValid(void);

/* =============================================================================
 * END OF FILE
 * ============================================================================= */

#endif /* RTE_DOORCONTROLSWC_H */
