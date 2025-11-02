# === Configuration ===
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lreadline
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TARGET = $(BIN_DIR)/myshell

# === Source and Object Files ===
SRC = $(SRC_DIR)/builtins.c $(SRC_DIR)/execute.c $(SRC_DIR)/history.c $(SRC_DIR)/main.c $(SRC_DIR)/shell.c
OBJ = $(OBJ_DIR)/builtins.o $(OBJ_DIR)/execute.o $(OBJ_DIR)/history.o $(OBJ_DIR)/main.o $(OBJ_DIR)/shell.o

# === Rules ===
all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo " Build complete: $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# === Utility Targets ===
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned build files."

rebuild: clean all

.PHONY: all clean run 
