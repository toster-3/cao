MODE     := dev
ifeq ($(MODE), dev)
	OFLAGS := -g -O0 -DDEBUG
else
	OFLAGS := -O3 -march=native
endif
CPPFLAGS :=
CFLAGS   := -std=c11 -pedantic -Wall $(OFLAGS)
LDFLAGS  := -lc

CC = clang

TARGET_EXE := cao

BUILD_DIR := build
SRC_DIR   := src
INC_DIR   := src

CPPFLAGS += $(addprefix -I,$(INC_DIR))

SRC := $(shell find $(SRC_DIR) -name '*.c')
OBJ := $(addprefix $(BUILD_DIR)/, $(notdir $(SRC:.c=.o)))

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET_EXE)

$(BUILD_DIR)/$(TARGET_EXE): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

install: all
	strip $(BUILD_DIR)/$(TARGET_EXE)
	cp $(BUILD_DIR)/$(TARGET_EXE) ~/.local/bin

clean:
	rm -r $(BUILD_DIR)

.PHONY: all clean install
