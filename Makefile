# OH ym god

.DEFAULT_GOAL := all

ASM      := nasm
CROSS    := i386-elf
CC       := $(CROSS)-gcc
LD       := $(CROSS)-ld
OBJCOPY  := $(CROSS)-objcopy
BUN      := bun

SRC_DIR  := src
BUILD    := build
INC_DIR  := include

GREEN  := $(shell printf '\033[0;32m')
BLUE   := $(shell printf '\033[0;34m')
CYAN   := $(shell printf '\033[0;36m')
YELLOW := $(shell printf '\033[1;33m')
RESET  := $(shell printf '\033[0m')

LOCAL_TOOLCHAIN := $(CURDIR)/tools/bin/bin
REQUIRED_TOOLS := i386-elf-gcc i386-elf-ld i386-elf-objcopy
export PATH := $(LOCAL_TOOLCHAIN):$(PATH)

check_toolchain:
	@missing=""; \
	for t in $(REQUIRED_TOOLS); do \
		if command -v $$t >/dev/null 2>&1; then \
			continue; \
		elif [ -x "$(LOCAL_TOOLCHAIN)/$$t" ]; then \
			echo "added $(LOCAL_TOOLCHAIN) to PATH for $$t"; \
		else \
			missing="$$missing $$t"; \
		fi; \
	done; \
	if [ "$$missing" != "" ]; then \
		echo "$(YELLOW)missing toolchain:$$missing$(RESET)"; \
		echo "run: make toolchain"; \
		exit 1; \
	fi

toolchain:
	@echo "$(CYAN)installing local i386 toolchain$(RESET)"
	@cd tools && ./build-i386-elf.sh
	@echo "$(GREEN)[OK]$(RESET) toolchain ready"

ifeq ($(shell command -v $(BUN) 2>/dev/null),)
  $(error $(BUN) not found on PATH (needed to bundle the TypeScript))
endif

QJS_DIR := third_party/quickjs
GCC_INC := $(shell $(CC) -print-file-name=include)

STACK_TOP := $(shell sed -n 's/^#define STACK_TOP[[:space:]]*\(0x[0-9A-Fa-f]*\)UL.*/\1/p' $(SRC_DIR)/kernel/kernel.h | head -1)
HEAP_END  := $(shell sed -n 's/^#define HEAP_END[[:space:]]*\(0x[0-9A-Fa-f]*\)UL.*/\1/p'  $(SRC_DIR)/kernel/kernel.h | head -1)
ASM_MEM_DEFS := -DSTACK_TOP=$(STACK_TOP)

FIRST_C_FLAGS := -m32 -ffreestanding -fno-pie -fno-stack-protector \
           -fno-exceptions -nostdlib -nostdinc -I $(INC_DIR) \
           -I $(QJS_DIR) \
           -isystem $(GCC_INC) -std=c17 -Wall -Wextra \
           -Wno-unused-parameter -g
C_FLAGS := $(FIRST_C_FLAGS) -DHEAP_END_ADDR=$(HEAP_END)
QJS_FLAGS := $(C_FLAGS) -O2 -w -D__EMSCRIPTEN__ -DCONFIG_VERSION=\"2026-06-04\"

LIBGCC := $(realpath $(shell $(CC) -print-libgcc-file-name 2>/dev/null))
LIBGCC_LIBS := $(if $(LIBGCC),-L$(dir $(LIBGCC)) -lgcc)

SECTOR_SIZE := 512

QJS_OBJS := $(BUILD)/qjs/quickjs.o \
            $(BUILD)/qjs/cutils.o \
            $(BUILD)/qjs/libregexp.o \
            $(BUILD)/qjs/libunicode.o \
            $(BUILD)/qjs/dtoa.o

KERNEL_OBJS := $(BUILD)/kernel/entry.o \
               $(BUILD)/kernel/isr.o \
               $(BUILD)/kernel/kernel.o \
               $(BUILD)/kernel/klib.o \
               $(BUILD)/kernel/math.o \
               $(BUILD)/kernel/js.o \
               $(BUILD)/kernel/tsbundle.o

TS_SOURCES := $(shell find $(SRC_DIR)/ts -type f -name '*.ts')
TS_BUNDLE  := $(BUILD)/ts/main.js
TS_BUNDLE_C := $(BUILD)/kernel/tsbundle.c

$(BUILD):
	@mkdir -p $(BUILD)

all: check_toolchain $(BUILD)/os.img
	@echo "$(GREEN)[OK]$(RESET) built $(BUILD)/os.img"

$(TS_BUNDLE): $(TS_SOURCES) src/ts/tsconfig.json | $(BUILD)
	@mkdir -p $(dir $@)
	@echo "$(BLUE)[TS]$(RESET) bundling typescript shell"
	@$(BUN) build $(SRC_DIR)/ts/main.ts --bundle --format=iife \
	    --target=browser --minify --outfile=$@

$(TS_BUNDLE_C): $(TS_BUNDLE) tools/embed.ts $(BUILD)
	@mkdir -p $(dir $@)
	@echo "$(CYAN)[EMBED]$(RESET) embedding typescript bundle"
	@$(BUN) tools/embed.ts $(TS_BUNDLE) $@

$(BUILD)/kernel/tsbundle.o: $(TS_BUNDLE_C)
	@echo "$(GREEN)[CC]$(RESET) $<"
	@$(CC) $(C_FLAGS) -c $< -o $@

$(BUILD)/kernel/%.o: $(SRC_DIR)/kernel/%.c | $(BUILD)
	@mkdir -p $(dir $@)
	@echo "$(GREEN)[CC]$(RESET) $<"
	@$(CC) $(C_FLAGS) -c $< -o $@

$(BUILD)/kernel/%.o: $(SRC_DIR)/kernel/%.asm | $(BUILD)
	@mkdir -p $(dir $@)
	@echo "$(CYAN)[ASM]$(RESET) $<"
	@$(ASM) $(ASM_MEM_DEFS) -f elf $< -o $@

$(BUILD)/qjs/%.o: $(QJS_DIR)/%.c | $(BUILD)
	@mkdir -p $(dir $@)
	@echo "$(GREEN)[CC]$(RESET) $<"
	@$(CC) $(QJS_FLAGS) -c $< -o $@

$(BUILD)/kernel.elf: $(KERNEL_OBJS) $(QJS_OBJS)
	@echo "$(BLUE)[LD]$(RESET) linking kernel"
	@$(LD) -m elf_i386 -T linker.ld -o $@ $(KERNEL_OBJS) $(QJS_OBJS) \
	    $(LIBGCC_LIBS) -Map $(BUILD)/kernel.map

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	@echo "$(BLUE)[OBJCOPY]$(RESET) $<"
	@$(OBJCOPY) -O binary $< $@

BOOT_ASMS := $(SRC_DIR)/boot/boot.asm $(SRC_DIR)/boot/print.asm \
    $(SRC_DIR)/boot/pm_copy.asm $(SRC_DIR)/boot/protected_mode.asm \
    $(SRC_DIR)/boot/gdt.asm

$(BUILD)/boot_sect.bin: $(BOOT_ASMS) $(BUILD)/kernel.bin
	@SECTORS=$$(stat -c%s $(BUILD)/kernel.bin | \
	    awk -v sz=$(SECTOR_SIZE) '{printf "%d", ($$1+sz-1)/sz}'); \
	echo "$(CYAN)[BOOT]$(RESET) kernel is $$SECTORS sectors"; \
	$(ASM) $(ASM_MEM_DEFS) $< -DNUM_KERNEL_SECTORS=$$SECTORS -f bin \
	    -I $(SRC_DIR)/boot/ -o $@

$(BUILD)/os.img: $(BUILD)/boot_sect.bin $(BUILD)/kernel.bin
	@echo "$(YELLOW)[CAT]$(RESET) building disk image"
	@cat $^ > $@
	@truncate -s 1474560 $@   # pad to 1.44MB so QEMU picks 80x2x18 geometry
	@echo "$(GREEN)[OK]$(RESET) wrote $(BUILD)/os.img ($$(stat -c%s $@) bytes)"

run: $(BUILD)/os.img
	@echo "$(CYAN)[QEMU]$(RESET) typeos in qemu (ctrl-alt-g to release mouse)"
	@qemu-system-i386 -drive file=$(BUILD)/os.img,format=raw,if=floppy \
	    -boot a -m 32

clean:
	@echo "$(YELLOW)[CLEAN]$(RESET) removing build dir"
	@rm -rf $(BUILD)

.PHONY: all run test clean check_toolchain toolchain