CC = gcc
AR = ar
CFLAGS = -Wall -fPIC -I. -Isrc
LDFLAGS = -shared
BUILD_DIR ?= build
OBJ_DIR = $(BUILD_DIR)/obj
INSTALL_PREFIX ?= /usr/local

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRCS))

LIB = $(BUILD_DIR)/libbeaker
HEADER = beaker.h

.PHONY: all clean install uninstall

all: $(LIB).so $(LIB).a


$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(LIB).a: $(OBJS)
	@echo "Linking shared library $@..."
	$(ar) rcs $@ $^
	@echo "Successfully built $@"

$(LIB).so: $(OBJS)
	@echo "Linking shared library $@..."
	$(CC) $(LDFLAGS) $(OBJS) -o $@ -lm
	@echo "Successfully built $@"

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up object files and shared library..."
	@rm -rf $(OBJ_DIR) $(LIB)
	@echo "Clean complete."

install: all
	@echo "Installing $(LIB) to $(INSTALL_PREFIX)/lib"
	@cp $(LIB) $(INSTALL_PREFIX)/lib/
	@ldconfig
	@echo "Installing $(HEADER) to $(INSTALL_PREFIX)/include"
	@cp $(HEADER) $(INSTALL_PREFIX)/include
	@echo "Installation complete."

uninstall:
	@echo "Uninstalling $(LIB) from $(INSTALL_PREFIX)/lib..."
	@rm -f $(INSTALL_PREFIX)/lib/$(LIB)
	@ldconfig
	@echo "Removing $(HEADER) from $(INSTALL_PREFIX)/include"
	@rm -f $(INSTALL_PREFIX)/include/$(HEADER)
	@echo "Uninstallation complete."
