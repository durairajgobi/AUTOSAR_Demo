# Makefile
# Door Control System - Build Configuration
#
# Quick build without CMake
# Usage: make [target]
#
# Targets:
#   make              - Build test executable
#   make app          - Build firmware application
#   make clean        - Remove build artifacts
#   make test         - Run unit tests
#   make help         - Show this help

# =============================================================================
# COMPILER SETTINGS
# =============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -Iinclude
LDFLAGS = 
LIBS = 

# Debug mode
DEBUG = 1
ifeq ($(DEBUG), 1)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2 -DNDEBUG
endif

# Optional: Enable simulation mode
SIM = 1
ifeq ($(SIM), 1)
    CFLAGS += -DSIMULATION_MODE=1
endif

# =============================================================================
# FILE STRUCTURE
# =============================================================================

SRC_DIR = .
OBJ_DIR = build/obj
BIN_DIR = build/bin
DEP_DIR = build/dep

# Source files
COMMON_SRCS = 06_SampleApplicationCode.c Diagnostics.c Hal.c
TEST_SRCS = test_DoorControl.c
APP_SRCS = main.c

# Object files
COMMON_OBJS = $(addprefix $(OBJ_DIR)/, $(COMMON_SRCS:.c=.o))
TEST_OBJS = $(OBJ_DIR)/test_DoorControl.o
APP_OBJS = $(OBJ_DIR)/main.o

# Dependency files
DEPS = $(patsubst $(OBJ_DIR)/%.o, $(DEP_DIR)/%.d, $(COMMON_OBJS) $(TEST_OBJS) $(APP_OBJS))

# Executables
TEST_EXE = $(BIN_DIR)/door_control_test
APP_EXE = $(BIN_DIR)/door_control_app

# =============================================================================
# RULES
# =============================================================================

.PHONY: all test app clean help run

# Default target: build test
all: test

# Build test executable
test: $(TEST_EXE)
	@echo "✓ Test executable built: $(TEST_EXE)"

# Build application firmware
app: $(APP_EXE)
	@echo "✓ Application executable built: $(APP_EXE)"

# Build directories
$(OBJ_DIR) $(BIN_DIR) $(DEP_DIR):
	@mkdir -p $@

# Object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

# Test executable
$(TEST_EXE): $(COMMON_OBJS) $(TEST_OBJS) | $(BIN_DIR)
	@echo "Linking test executable..."
	@$(CC) $(LDFLAGS) $^ $(LIBS) -o $@
	@echo "✓ Built: $@"

# Application executable
$(APP_EXE): $(COMMON_OBJS) $(APP_OBJS) | $(BIN_DIR)
	@echo "Linking application..."
	@$(CC) $(LDFLAGS) $^ $(LIBS) -o $@
	@echo "✓ Built: $@"

# Include dependencies
-include $(DEPS)

# =============================================================================
# TARGETS
# =============================================================================

# Run tests
run: test
	@echo ""
	@echo "Running unit tests..."
	@echo "===================="
	@$(TEST_EXE)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build/
	@echo "✓ Cleaned"

# Show statistics
stats:
	@echo "Code Statistics:"
	@echo "================"
	@wc -l $(COMMON_SRCS) $(TEST_SRCS) $(APP_SRCS) *.h
	@echo ""
	@echo "File sizes:"
	@du -h *.c *.h

# Show help
help:
	@echo ""
	@echo "Door Control System - Makefile"
	@echo "==============================="
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  test      - Build unit tests (default)"
	@echo "  app       - Build firmware application"
	@echo "  run       - Build and run tests"
	@echo "  clean     - Remove build artifacts"
	@echo "  stats     - Show code statistics"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  DEBUG=1   - Enable debug symbols (default)"
	@echo "  DEBUG=0   - Disable debug symbols (release)"
	@echo "  SIM=1     - Enable simulation mode (default)"
	@echo "  SIM=0     - Disable simulation mode"
	@echo ""
	@echo "Examples:"
	@echo "  make                  # Build tests"
	@echo "  make app              # Build application"
	@echo "  make run              # Build and run tests"
	@echo "  make clean            # Remove build directory"
	@echo "  make DEBUG=0 SIM=0    # Release build without simulation"
	@echo ""

# =============================================================================
# SPECIAL TARGETS
# =============================================================================

# Install (example for Linux)
install: app
	@echo "Installing..."
	@mkdir -p /usr/local/bin
	@cp $(APP_EXE) /usr/local/bin/door_control
	@mkdir -p /usr/local/include
	@cp *.h /usr/local/include/
	@echo "✓ Installed to /usr/local"

# Uninstall
uninstall:
	@echo "Uninstalling..."
	@rm -f /usr/local/bin/door_control
	@rm -f /usr/local/include/DoorControl.h
	@rm -f /usr/local/include/Diagnostics.h
	@echo "✓ Uninstalled"

# Format code
format:
	@echo "Formatting code..."
	@clang-format -i *.c *.h
	@echo "✓ Formatted"

# Check code with static analyzer
check:
	@echo "Running static analysis..."
	@cppcheck --enable=all --suppress=missingIncludeSystem *.c *.h
	@echo "✓ Analysis complete"

# Create archive
archive: clean
	@echo "Creating archive..."
	@tar czf door_control_system.tar.gz \
		--exclude=build \
		--exclude=.git \
		*.c *.h *.md Makefile CMakeLists.txt *.arxml
	@echo "✓ Archive created: door_control_system.tar.gz"

# =============================================================================
# DEBUG TARGETS
# =============================================================================

# Show build variables
show-vars:
	@echo "Build Variables:"
	@echo "================"
	@echo "CC:         $(CC)"
	@echo "CFLAGS:     $(CFLAGS)"
	@echo "DEBUG:      $(DEBUG)"
	@echo "SIM:        $(SIM)"
	@echo "TEST_EXE:   $(TEST_EXE)"
	@echo "APP_EXE:    $(APP_EXE)"
	@echo ""

# Verbose build
verbose: CFLAGS += -v
verbose: clean test
	@echo "Verbose build complete"

# =============================================================================
# PHONY TARGETS DECLARATION
# =============================================================================

.PHONY: all test app clean help run install uninstall format check archive show-vars verbose stats

# Default goal
.DEFAULT_GOAL := all
