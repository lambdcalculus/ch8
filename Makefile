# target
TARGET_NAME := ch8

# compiler stuff
CWARNFLAGS := -Wall -Wextra -Wfloat-equal -Wundef -Wpointer-arith \
              -Wwrite-strings -Wunused -Wconversion
CFLAGS     ?= -std=c11
DEBUGFLAGS := -g $(CWARNFLAGS)
LIBFLAGS   := $(shell pkg-config sdl2 --libs)

# directory stuff
BUILD_DIR := build
SRC_DIR   := src
OBJ_DIR   := build/obj

# file stuff
SOURCES   = $(wildcard $(SRC_DIR)/*.c)
OBJECTS   = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(notdir $(SOURCES))))
TARGET   := $(BUILD_DIR)/$(TARGET_NAME)

default: all


# making object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

# linking object files
$(TARGET): $(OBJECTS) 
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBFLAGS) -o $@

# phony rules
.PHONY: all
all: makedir $(TARGET)

.PHONY: makedir
makedir:
	mkdir -p $(BUILD_DIR) $(OBJ_DIR)

.PHONY: debug
debug: CFLAGS += $(DEBUGFLAGS)
debug: $(TARGET)

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
	rm $(SRC_DIR)/xdg*

.PHONY: distclean
distclean:
	rm -rf $(BUILD_DIR)