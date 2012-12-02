# Set this to the name of your program
TARGET = file_loc
TARGET_DIR = /data/misc

# Edit this variable to point to all
# of your sources files (make sure
# to put a complete relative path)
SOURCES = file_loc.c

# -----------------------------------------------
# 
# Don't touch things below here unless
# you know what you're doing :-)
# 
KERNEL_SRC=../../../android-tegra-3.1
CRUNTIME = ../../asm/crtbegin_dynamic.S \
           ../../asm/crtend.S
OBJECTS = $(SOURCES:%.c=%.c.o) $(CRUNTIME:%.S=%.S.o)
INCLUDE = -I. -I../../inc -I../../inc/bionic \
	  -I$(KERNEL_SRC)/usr/include \
	  -I$(KERNEL_SRC)/include -I$(KERNEL_SRC)/arch/arm/include
EXTRA_LIBS = -lc
CFLAGS = -g -O2 -Wall $(INCLUDE) $(EXTRA_CFLAGS)
LDFLAGS = --entry=_start --dynamic-linker /system/bin/linker \
          -nostdlib -rpath /system/lib -rpath ../../system/lib \
          -L ../../system/lib $(EXTRA_LIBS)
CC = arm-none-linux-gnueabi-gcc
LD = arm-none-linux-gnueabi-ld

default: $(SOURCES) $(TARGET)

kernel_headers:
	cd $(KERNEL_SRC); make ARCH=arm headers_install

$(TARGET): kernel_headers $(OBJECTS)
	@echo [ARM-LD] $@
	@$(LD) $(LDFLAGS) $(OBJECTS) -o $@

%.c.o: %.c
	@echo [ARM-CC] $<...
	@$(CC) -c $(CFLAGS) $< -o $@

%.S.o: %.S
	@echo [ARM-ASM] $<...
	@$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean kernel_headers
clean:
	@echo [CLEAN] removing object files + $(TARGET)...
	@rm -f $(OBJECTS) $(TARGET)

%_emu: ADB_OPT = -e
%_emu: ADB_TGT = Android Emulator
%_dev: ADB_OPT = -d
%_dev: ADB_TGT = Android Device

install_%: $(TARGET)
	@echo [INSTALL] moving $(TARGET) to $(ADB_TGT) [$(TARGET_DIR)]...
	@adb $(ADB_OPT) remount >/dev/null 2>&1
	@adb $(ADB_OPT) push $(TARGET) $(TARGET_DIR)
	@adb $(ADB_OPT) shell chmod 6755 $(TARGET_DIR)/$(TARGET)

user_install_%: $(TARGET)
	@echo [INSTALL:user] moving $(TARGET) to $(ADB_TGT) [$(TARGET_DIR)]...
	@adb $(ADB_OPT) remount > /dev/null 2>&1
	@adb $(ADB_OPT) push $(TARGET) $(TARGET_DIR)
	@adb $(ADB_OPT) shell chmod 0750 $(TARGET_DIR)/$(TARGET)
	@adb $(ADB_OPT) shell chown compass.system $(TARGET_DIR)/$(TARGET)

run_%: install_%
	@echo [RUN] calling $(TARGET_DIR)/$(TARGET) on $(ADB_TGT) [$(TARGET_DIR)]...
	@echo "      starting $(TARGET)... "
	@echo "      (you may have to Ctrl-C to get your shell back)"
	@echo "$(TARGET_DIR)/$(TARGET)" | adb $(ADB_OPT) shell

.PHONY:

