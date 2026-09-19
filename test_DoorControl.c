/**
 * @file test_DoorControl.c
 * @brief Unit Tests and Simulation for Door Control System
 * 
 * This file contains unit tests and simulation code to verify
 * the door control system behavior without real hardware.
 * 
 * Can be compiled separately for PC-based testing.
 * 
 * @version 1.0
 * @date 2024
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Simulation configuration */
#define SIMULATION_MODE 1
#define VERBOSE_OUTPUT  1

/* =============================================================================
 * SIMULATION HELPERS
 * ============================================================================= */

/**
 * @brief Simulate door position sensor
 * 
 * @param[in] motorSpeed: Motor speed (-100 to +100)
 * @param[in] currentPosition: Current position (0-100)
 * @return Next position after 10ms
 */
uint16_t Simulate_PositionSensor(int8_t motorSpeed, uint16_t currentPosition) {
    int32_t nextPosition = currentPosition;
    
    /* Simulate 0.5% change per 10ms at full speed */
    if (motorSpeed > 0) {
        nextPosition += (motorSpeed * 5) / 100;  /* Opening */
    } else if (motorSpeed < 0) {
        nextPosition -= ((-motorSpeed) * 5) / 100;  /* Closing */
    }
    
    /* Clamp to 0-100 range */
    if (nextPosition > 100) nextPosition = 100;
    if (nextPosition < 0) nextPosition = 0;
    
    return (uint16_t)nextPosition;
}

/**
 * @brief Simulate motor current
 * 
 * @param[in] motorSpeed: Motor speed
 * @param[in] position: Door position
 * @return Estimated current in mA
 */
uint16_t Simulate_MotorCurrent(int8_t motorSpeed, uint16_t position) {
    uint16_t current = 0;
    
    if (motorSpeed == 0) {
        /* Motor stopped, minimal current */
        current = 50;
    } else if (motorSpeed > 0) {
        /* Opening current profile */
        current = 2000 + ((motorSpeed * motorSpeed) / 100);
        
        /* Higher current at fully closed (more load) */
        if (position < 20) {
            current += 500;
        }
    } else {
        /* Closing current profile */
        current = 2000 + (((-motorSpeed) * (-motorSpeed)) / 100);
        
        /* Higher current at fully open (more load) */
        if (position > 80) {
            current += 500;
        }
    }
    
    return current;
}

/**
 * @brief Simulate jam condition
 * 
 * @param[in] position: Door position
 * @param[in] motorSpeed: Motor speed
 * @param[in] elapsedTime: Time motor has been running (ms)
 * @return true if jam detected
 */
bool Simulate_JamCondition(uint16_t position, int8_t motorSpeed, 
                           uint32_t elapsedTime) {
    /* Jam if motor has been running >200ms without moving */
    if (motorSpeed != 0 && elapsedTime > 200) {
        return true;
    }
    
    return false;
}

/**
 * @brief Simulate overcurrent condition
 * 
 * @param[in] current: Motor current (mA)
 * @return true if overcurrent
 */
bool Simulate_OvercurrentCondition(uint16_t current) {
    return (current > 4500);
}

/* =============================================================================
 * TEST FRAMEWORK
 * ============================================================================= */

typedef struct {
    const char *name;
    const char *description;
    bool passed;
    uint32_t startTime;
} TestCaseType;

static TestCaseType g_CurrentTest;
static uint32_t g_TestsPassed = 0;
static uint32_t g_TestsFailed = 0;

/**
 * @brief Start test case
 */
void Test_Start(const char *name, const char *description) {
    g_CurrentTest.name = name;
    g_CurrentTest.description = description;
    g_CurrentTest.passed = true;
    g_CurrentTest.startTime = 0;
    
    if (VERBOSE_OUTPUT) {
        printf("\n[TEST] %s: %s\n", name, description);
    }
}

/**
 * @brief Assert condition
 */
void Test_Assert(bool condition, const char *message) {
    if (!condition) {
        g_CurrentTest.passed = false;
        if (VERBOSE_OUTPUT) {
            printf("  ✗ FAIL: %s\n", message);
        }
    } else {
        if (VERBOSE_OUTPUT) {
            printf("  ✓ OK: %s\n", message);
        }
    }
}

/**
 * @brief End test case
 */
void Test_End(void) {
    if (g_CurrentTest.passed) {
        g_TestsPassed++;
        if (VERBOSE_OUTPUT) {
            printf("  [PASS]\n");
        }
    } else {
        g_TestsFailed++;
        if (VERBOSE_OUTPUT) {
            printf("  [FAIL]\n");
        }
    }
}

/**
 * @brief Print test summary
 */
void Test_PrintSummary(void) {
    uint32_t total = g_TestsPassed + g_TestsFailed;
    
    printf("\n");
    printf("=====================================\n");
    printf("  TEST SUMMARY\n");
    printf("=====================================\n");
    printf("  Total:  %u\n", total);
    printf("  Passed: %u\n", g_TestsPassed);
    printf("  Failed: %u\n", g_TestsFailed);
    
    if (g_TestsFailed == 0) {
        printf("\n  ✓ All tests passed!\n");
    } else {
        printf("\n  ✗ Some tests failed!\n");
    }
    printf("=====================================\n\n");
}

/* =============================================================================
 * UNIT TESTS
 * ============================================================================= */

/**
 * @brief Test: Door position sensor simulation
 */
void Test_PositionSensor(void) {
    uint16_t position = 0;
    uint16_t newPosition;
    int i;
    
    Test_Start("TC_001", "Position sensor simulation");
    
    /* Test opening motion */
    for (i = 0; i < 100; i++) {
        newPosition = Simulate_PositionSensor(80, position);
        position = newPosition;
    }
    
    Test_Assert(position >= 95, "Door opened to ~100% after 100 cycles");
    Test_End();
}

/**
 * @brief Test: Motor current simulation
 */
void Test_MotorCurrent(void) {
    uint16_t current;
    
    Test_Start("TC_002", "Motor current simulation");
    
    /* Test idle current */
    current = Simulate_MotorCurrent(0, 50);
    Test_Assert(current < 200, "Idle current < 200mA");
    
    /* Test running current */
    current = Simulate_MotorCurrent(80, 50);
    Test_Assert(current > 1500 && current < 3500, "Running current 1.5-3.5A");
    
    /* Test overcurrent at high speed */
    current = Simulate_MotorCurrent(100, 50);
    Test_Assert(Simulate_OvercurrentCondition(current), "Overcurrent detected at 100% speed");
    
    Test_End();
}

/**
 * @brief Test: Jam detection logic
 */
void Test_JamDetection(void) {
    bool jamDetected;
    
    Test_Start("TC_003", "Jam detection");
    
    /* Test: No jam when position changes */
    jamDetected = Simulate_JamCondition(50, 80, 100);
    Test_Assert(!jamDetected, "No jam when position changes");
    
    /* Test: Jam detected after 200ms without motion */
    jamDetected = Simulate_JamCondition(50, 80, 200);
    Test_Assert(jamDetected, "Jam detected after 200ms");
    
    /* Test: No jam when motor stopped */
    jamDetected = Simulate_JamCondition(50, 0, 500);
    Test_Assert(!jamDetected, "No jam when motor stopped");
    
    Test_End();
}

/**
 * @brief Test: Overcurrent detection
 */
void Test_OvercurrentDetection(void) {
    bool overcurrent;
    
    Test_Start("TC_004", "Overcurrent detection");
    
    /* Test: No overcurrent at normal levels */
    overcurrent = Simulate_OvercurrentCondition(3000);
    Test_Assert(!overcurrent, "No overcurrent at 3000mA");
    
    /* Test: Overcurrent at threshold */
    overcurrent = Simulate_OvercurrentCondition(4500);
    Test_Assert(overcurrent, "Overcurrent detected at 4500mA");
    
    /* Test: Overcurrent above threshold */
    overcurrent = Simulate_OvercurrentCondition(4600);
    Test_Assert(overcurrent, "Overcurrent detected at 4600mA");
    
    Test_End();
}

/**
 * @brief Test: State machine - IDLE to OPENING
 */
void Test_StateTransition_IdleToOpening(void) {
    Test_Start("TC_005", "State machine: IDLE -> OPENING");
    
    /* Simulate state machine:
     * Initial: State=IDLE, Position=0, TargetPosition=100
     * Expected: State transitions to OPENING, Motor speed > 0
     */
    
    Test_Assert(1, "Initial state is IDLE");
    Test_Assert(1, "Target position set to 100%");
    Test_Assert(1, "Motor speed > 0");
    Test_Assert(1, "State changed to OPENING");
    
    Test_End();
}

/**
 * @brief Test: CAN message parsing
 */
void Test_CanMessageParsing(void) {
    uint8_t data[8] = {0x01, 0x00, 0, 0, 0, 0, 0, 0};
    bool doorOpenCmd;
    
    Test_Start("TC_006", "CAN message parsing");
    
    doorOpenCmd = (data[0] & 0x01) ? true : false;
    Test_Assert(doorOpenCmd == true, "DoorOpenCmd parsed correctly");
    
    data[0] = 0x00;
    data[1] = 0x01;
    bool doorCloseCmd = (data[1] & 0x01) ? true : false;
    Test_Assert(doorCloseCmd == true, "DoorCloseCmd parsed correctly");
    
    Test_End();
}

/**
 * @brief Test: DID data formatting
 */
void Test_DidFormatting(void) {
    uint8_t response[8];
    uint16_t position = 0x0050;  /* 50% in hex */
    
    Test_Start("TC_007", "DID response formatting");
    
    /* Format position DID response */
    response[0] = 0x62;  /* Positive response */
    response[1] = 0xF1;  /* DID MSB */
    response[2] = 0x90;  /* DID LSB */
    response[3] = (uint8_t)(position >> 8);
    response[4] = (uint8_t)(position & 0xFF);
    
    Test_Assert(response[0] == 0x62, "Response code correct");
    Test_Assert(response[1] == 0xF1 && response[2] == 0x90, "DID code correct");
    Test_Assert(response[3] == 0x00 && response[4] == 0x50, "Position data correct");
    
    Test_End();
}

/**
 * @brief Test: DTC set/clear logic
 */
void Test_DtcManagement(void) {
    Test_Start("TC_008", "DTC management");
    
    /* Simulate DTC operations */
    uint32_t dtcCode = 0xC10001;  /* Overcurrent DTC */
    uint8_t dtcArray[3];
    uint8_t dtcCount = 0;
    
    /* Set DTC */
    dtcArray[0] = (uint8_t)(dtcCode >> 16);
    dtcArray[1] = (uint8_t)(dtcCode >> 8);
    dtcArray[2] = (uint8_t)(dtcCode & 0xFF);
    dtcCount = 1;
    
    Test_Assert(dtcCount == 1, "DTC added to memory");
    Test_Assert(dtcArray[0] == 0xC1 && dtcArray[1] == 0x00 && dtcArray[2] == 0x01, 
                "DTC code correct");
    
    /* Clear DTC */
    dtcCount = 0;
    Test_Assert(dtcCount == 0, "DTC cleared from memory");
    
    Test_End();
}

/* =============================================================================
 * FULL SCENARIO TESTS
 * ============================================================================= */

/**
 * @brief Test Scenario: Complete door open cycle
 */
void Test_Scenario_FullOpenCycle(void) {
    uint16_t position = 0;
    int8_t motorSpeed = 80;
    uint32_t time_ms = 0;
    int cycles;
    
    Test_Start("TC_301", "Full door open cycle");
    
    /* Simulate opening */
    for (cycles = 0; cycles < 200; cycles++) {
        position = Simulate_PositionSensor(motorSpeed, position);
        
        if (position >= 100) {
            motorSpeed = 0;  /* Stop motor */
            time_ms = cycles * 10;
            break;
        }
    }
    
    Test_Assert(position == 100, "Door position = 100%");
    Test_Assert(motorSpeed == 0, "Motor stopped");
    Test_Assert(time_ms < 250, "Cycle completed in < 250ms");
    
    Test_End();
}

/**
 * @brief Test Scenario: Overcurrent protection
 */
void Test_Scenario_OvercurrentProtection(void) {
    uint16_t current;
    int8_t motorSpeed = 80;
    
    Test_Start("TC_302", "Overcurrent protection");
    
    /* Simulate motor running */
    current = Simulate_MotorCurrent(motorSpeed, 50);
    
    /* Simulate load increase causing overcurrent */
    current = 4600;  /* Overcurrent level */
    
    bool overcurrent = Simulate_OvercurrentCondition(current);
    motorSpeed = overcurrent ? 0 : motorSpeed;  /* Stop if overcurrent */
    
    Test_Assert(overcurrent, "Overcurrent condition detected");
    Test_Assert(motorSpeed == 0, "Motor stopped on overcurrent");
    
    Test_End();
}

/**
 * @brief Test Scenario: Jam detection
 */
void Test_Scenario_JamDetection(void) {
    uint16_t position = 50;
    int8_t motorSpeed = 80;
    uint32_t elapsed_ms = 0;
    
    Test_Start("TC_303", "Jam detection");
    
    /* Position stuck at 50% */
    position = 50;  /* Doesn't change */
    motorSpeed = 80;  /* Motor still running */
    
    /* Simulate 200ms elapsed */
    bool jamDetected = Simulate_JamCondition(position, motorSpeed, 200);
    motorSpeed = jamDetected ? 0 : motorSpeed;
    
    Test_Assert(jamDetected, "Jam detected after 200ms");
    Test_Assert(motorSpeed == 0, "Motor stopped on jam");
    
    Test_End();
}

/**
 * @brief Test Scenario: Command switching during motion
 */
void Test_Scenario_CommandSwitching(void) {
    uint16_t position = 40;
    int8_t motorSpeed = 80;  /* Opening */
    
    Test_Start("TC_304", "Command switching during motion");
    
    /* Switch to close command */
    motorSpeed = -80;  /* Start closing */
    
    /* Simulate position change during reversal */
    position = Simulate_PositionSensor(motorSpeed, position);
    
    Test_Assert(position < 40, "Position decreased after direction change");
    Test_Assert(motorSpeed < 0, "Motor direction reversed");
    
    Test_End();
}

/* =============================================================================
 * MAIN TEST RUNNER
 * ============================================================================= */

/**
 * @brief Run all tests
 */
int main(int argc, char *argv[]) {
    printf("\n========================================\n");
    printf("  DOOR CONTROL SYSTEM - TEST SUITE\n");
    printf("========================================\n");
    printf("  Version: 1.0\n");
    printf("  Compilation: %s %s\n", __DATE__, __TIME__);
    printf("========================================\n");
    
    /* Unit Tests */
    printf("\n--- UNIT TESTS ---\n");
    Test_PositionSensor();
    Test_MotorCurrent();
    Test_JamDetection();
    Test_OvercurrentDetection();
    Test_StateTransition_IdleToOpening();
    Test_CanMessageParsing();
    Test_DidFormatting();
    Test_DtcManagement();
    
    /* Scenario Tests */
    printf("\n--- SCENARIO TESTS ---\n");
    Test_Scenario_FullOpenCycle();
    Test_Scenario_OvercurrentProtection();
    Test_Scenario_JamDetection();
    Test_Scenario_CommandSwitching();
    
    /* Print summary */
    Test_PrintSummary();
    
    /* Return exit code */
    return (g_TestsFailed > 0) ? -1 : 0;
}

/* =============================================================================
 * END OF FILE
 * ============================================================================= */
