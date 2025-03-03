###############################################################################
 #
 # Copyright (C) 2022-2023 Maxim Integrated Products, Inc. (now owned by
 # Analog Devices, Inc.),
 # Copyright (C) 2023-2024 Analog Devices, Inc.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #     http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 #
 ##############################################################################

# Makefile targets to flash with JLinkExe or OpenOCD

# ADAPTER_SN is debugger serial number
# This variable is optional, useful when multiple debuggers are connected
# It can be defined as an environment variable or in the Makefiles
#
# Example for JLINK: 229002239
# Example for CMSIS-DAP: 04090000068682d300000000000000000000000097969906
#
# ADAPTER_SN=229002239
# ADAPTER_SN=04090000068682d300000000000000000000000097969906

# Location of the OpenOCD install directory, tcl or scripts folder
OPENOCD_SCRIPTS ?= ${MAXIM_PATH}/Tools/OpenOCD/scripts

# Update with the connected adapter
OPENOCD_ADAPTER ?= cmsis-dap.cfg
# OPENOCD_ADAPTER ?= jlink.cfg

HEX_FILE        := ${BUILD_DIR}/${PROJECT}.hex
HEX_FILE_PATH   := ${HEX_FILE}
ifeq ($(OS),Windows_NT)
JLINKEXE        ?= JLink.exe
ECHO            ?= echo -e
OPENOCDEXE      ?= openocd.exe

# Determine if we can use cygpath to convert the path name
CYGPATH_AVAILABLE := 0
ifneq ($(findstring msys, $(_OS)), )
# NOTE:  "_OS" auto-detection comes from gcc.mk
# As a result, this file should be included AFTER gcc.mk
# This is done via the target's "MAX32xxx.mk" file.
CYGPATH_AVAILABLE := 1
endif

# Use cygpath to convert the path name
ifeq ($(CYGPATH_AVAILABLE),1)
HEX_FILE_PATH   := $(shell cygpath -wa $(HEX_FILE))
HEX_FILE_PATH   := $(subst \,/, $(HEX_FILE_PATH))
else
# $(warning Warning: cygpath unavailable to convert path name to Windows format)
endif

else
# Not Windows_NT
JLINKEXE        ?= JLinkExe
ECHO            ?= echo
OPENOCDEXE      ?= openocd
endif

JLINKEXE     += -if SWD -device ${TARGET_UC} -speed 10000
COMMAND_FILE := flash.jlinkexe

PHONY: flash.jlink
flash.jlink: mkbuildir ${HEX_FILE}
	@$(ECHO) "$(if $(ADAPTER_SN), "SelectEmuBySN $(ADAPTER_SN)",)\nr\nhalt\nLoadFile \
		${HEX_FILE_PATH},0\nr\ng\nexit\n" > ${COMMAND_FILE}
	@$(JLINKEXE) -NoGui 1 -CommandFile ${COMMAND_FILE}

PHONY: flash.openocd
flash.openocd: mkbuildir ${HEX_FILE}
	@$(OPENOCDEXE) -s ${OPENOCD_SCRIPTS} -f interface/${OPENOCD_ADAPTER} -f target/${TARGET_LC}.cfg \
		$(if $(ADAPTER_SN), "-c adapter serial $(ADAPTER_SN)",) \
		-c "program ${HEX_FILE_PATH} verify reset exit"
