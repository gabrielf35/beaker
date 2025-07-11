CC = gcc
CFLAGS = -Wall -fPIC -I. -Isrc
LDFLAGS = -shared
OBJ_DIR = obj

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRCS))

LIB = libbeaker.so
HEADER = beaker.h

.PHONY: all clean install uninstall

all: $(LIB)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(LIB): $(OBJS)
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
	@echo "Installing $(LIB) to /usr/local/lib"
	@cp $(LIB) /usr/local/lib/
	@ldconfig
	@echo "Installing $(HEADER) to /usr/local/include"
	@cp $(HEADER) /usr/local/include
	@echo "Installation complete."

uninstall:
	@echo "Uninstalling $(LIB) from /usr/local/lib..."
	@rm -f /usr/local/lib/$(LIB)
	@ldconfig
	@echo "Removing $(HEADER) from /usr/local/include"
	@rm -f /usr/local/include/$(HEADER)
	@echo "Uninstallation complete."