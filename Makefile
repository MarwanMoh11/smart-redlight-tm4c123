# Smart Red Light Violation Detection — Tiva C TM4C123GXL
# Toolchain: arm-none-eabi-gcc + TivaWare + FreeRTOS

# ===== Paths =====
TIVAWARE   := $(HOME)/ti/tivaware
FREERTOS   := $(HOME)/FreeRTOS-Kernel
PORT_DIR   := $(FREERTOS)/portable/GCC/ARM_CM4F
HEAP_DIR   := $(FREERTOS)/portable/MemMang

# ===== Toolchain =====
CC         := arm-none-eabi-gcc
LD         := arm-none-eabi-gcc
OBJCOPY    := arm-none-eabi-objcopy
SIZE       := arm-none-eabi-size

# ===== Target =====
TARGET     := redlight
PART       := TM4C123GH6PM

# ===== Architecture flags =====
CPU        := -mcpu=cortex-m4
FPU        := -mfpu=fpv4-sp-d16 -mfloat-abi=hard
ARCH       := -mthumb $(CPU) $(FPU)

# ===== Compiler flags =====
CFLAGS     := $(ARCH) -Os -ffunction-sections -fdata-sections \
              -Wall -Wextra -std=c11 -g -fno-unwind-tables -fno-exceptions -fno-asynchronous-unwind-tables \
              -DPART_$(PART) -Dgcc -DTARGET_IS_TM4C123_RB1 \
              -Iinc -I. \
              -I$(TIVAWARE) \
              -I$(FREERTOS)/include \
              -I$(PORT_DIR)

# ===== Linker flags =====
LDFLAGS    := $(ARCH) -nostartfiles -Wl,--gc-sections \
              -T tm4c123gh6pm.ld \
              -Wl,-Map=build/$(TARGET).map \
              --specs=nosys.specs

# ===== Libraries =====
LIBS       := -L$(TIVAWARE)/driverlib/gcc -ldriver \
              -lc -lgcc -lm

# ===== Source files =====
# Project sources (we'll add to this in Phase 3)
PROJ_SRCS  := startup_gcc.c \
              src/main.c \
              src/light_task.c \
              src/sensor_task.c

# FreeRTOS kernel sources
RTOS_SRCS  := $(FREERTOS)/tasks.c \
              $(FREERTOS)/queue.c \
              $(FREERTOS)/list.c \
              $(FREERTOS)/timers.c \
              $(FREERTOS)/event_groups.c \
              $(PORT_DIR)/port.c \
              $(HEAP_DIR)/heap_4.c

# Map all sources to object files in build/
PROJ_OBJS  := $(addprefix build/,$(notdir $(PROJ_SRCS:.c=.o)))
RTOS_OBJS  := $(addprefix build/,$(notdir $(RTOS_SRCS:.c=.o)))
ALL_OBJS   := $(PROJ_OBJS) $(RTOS_OBJS)

# Tell make where to find sources by basename
VPATH      := . src $(FREERTOS) $(PORT_DIR) $(HEAP_DIR)

# ===== Rules =====
.PHONY: all flash clean monitor size help

all: build/$(TARGET).bin

build:
	mkdir -p build

build/%.o: %.c | build
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

build/$(TARGET).elf: $(ALL_OBJS)
	@echo "  LD    $@"
	@$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo ""
	@$(SIZE) $@

build/$(TARGET).bin: build/$(TARGET).elf
	@echo "  BIN   $@"
	@$(OBJCOPY) -O binary $< $@

flash: build/$(TARGET).bin
	lm4flash $<

monitor:
	@echo "Opening /dev/ttyACM0 at 115200 baud (Ctrl+A then K to exit screen)"
	screen /dev/ttyACM0 115200

size: build/$(TARGET).elf
	$(SIZE) $

clean:
	rm -rf build

help:
	@echo "Targets:"
	@echo "  make           - build redlight.bin"
	@echo "  make flash     - build and flash to Tiva via lm4flash"
	@echo "  make monitor   - open serial console on /dev/ttyACM0"
	@echo "  make size      - print firmware size breakdown"
	@echo "  make clean     - remove build/"
