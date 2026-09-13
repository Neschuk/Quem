# ==============================================================================
# 								QUEM
# ==============================================================================

CC       := gcc
CFLAGS   := -Wall -Wextra -O3 -Iinclude
DEPFLAGS := -MMD -MP

SRC_DIR  := src
INC_DIR  := include
OBJ_DIR  := obj
BIN_NAME := quem

SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS     := $(OBJS:.o=.d)

GREEN    := \033[32m
YELLOW   := \033[33m
BLUE     := \033[34m
RESET    := \033[0m

.PHONY: all clean install uninstall

all: $(BIN_NAME)

$(BIN_NAME): $(OBJS)
	@echo -e "$(BLUE)[*] Linking object files...$(RESET)"
	@$(CC) $(CFLAGS) -o $@ $^
	@echo -e "$(GREEN)[+] Build successful! Executable created: ./$(BIN_NAME)$(RESET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo -e "$(YELLOW)[~] Compiling: $<$(RESET)"
	@$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@echo -e "$(YELLOW)[!] Cleaning build files...$(RESET)"
	@rm -rf $(OBJ_DIR) $(BIN_NAME)
	@echo -e "$(GREEN)[+] Clean complete.$(RESET)"

install: $(BIN_NAME)
	@echo -e "$(BLUE)[*] Installing $(BIN_NAME) to /usr/local/bin...$(RESET)"
	@sudo cp $(BIN_NAME) /usr/local/bin/
	@echo -e "$(GREEN)[+] Installation complete. You can now run 'sudo $(BIN_NAME)' anywhere.$(RESET)"

uninstall:
	@echo -e "$(YELLOW)[!] Removing $(BIN_NAME) from /usr/local/bin...$(RESET)"
	@sudo rm -f /usr/local/bin/$(BIN_NAME)
	@echo -e "$(GREEN)[+] Uninstallation complete.$(RESET)"ss