#
# Copyright (C) 2024 Bernd Herzog
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

cmake_minimum_required(VERSION 3.25)

MESSAGE(STATUS "Using toolchain file: ${CMAKE_SOURCE_DIR}/${CMAKE_TOOLCHAIN_FILE}")

#enable_language(C CXX ASM)

include(CheckCXXCompilerFlag)

project(uart_app CXX ASM)

# Compiler options here.
set(USE_OPT "-Os -g --specs=nano.specs --specs=nosys.specs")

# C specific options here (added to USE_OPT).
set(USE_COPT "-std=gnu99")

# C++ specific options here (added to USE_OPT).
check_cxx_compiler_flag("-std=c++20" cpp20_supported)
if(cpp20_supported)
	set(USE_CPPOPT "-std=c++20")
else()
	set(USE_CPPOPT "-std=c++17")
endif()
set(USE_CPPOPT "${USE_CPPOPT} -fno-rtti -fno-exceptions -Weffc++ -Wuninitialized -fno-use-cxa-atexit")

# Enable this if you want the linker to remove unused code and data
set(USE_LINK_GC yes)

# Linker extra options here.
#set(USE_LDOPT --nostartfiles)

# Enable this if you want link time optimizations (LTO) - this flag affects chibios only
set(USE_LTO no)

# If enabled, this option allows to compile the application in THUMB mode.
set(USE_THUMB yes)

# Enable this if you want to see the full log while compiling.
set(USE_VERBOSE_COMPILE no)

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Enables the use of FPU on Cortex-M4 (no, softfp, hard).
set(USE_FPU no)

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

# Define linker script file here
set(LDSCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/common/config/standalone_application_linker_script.ld)


# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
FILE(GLOB_RECURSE Sources_C ${CMAKE_CURRENT_LIST_DIR}/*.c)
FILE(GLOB_RECURSE Sources_C_COMMON ${CMAKE_CURRENT_LIST_DIR}common/*.c)
set(CSRC
	${Sources_C}
	${Sources_C_COMMON}
)

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
FILE(GLOB_RECURSE Sources_CPP ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
FILE(GLOB_RECURSE Sources_CPP_COMMON ${CMAKE_CURRENT_LIST_DIR}common/*.cpp)
set(CPPSRC
	${Sources_CPP}
	${Sources_CPP_COMMON}
)

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
set(ACSRC)

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
set(ACPPSRC)

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
set(TCSRC)

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
set(TCPPSRC)

# List ASM source files here
set(ASMSRC)

set(INCDIR
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/common
	${CMAKE_CURRENT_SOURCE_DIR}/common/ui
)

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

# TODO: Entertain using MCU=cortex-m0.small-multiply for LPC43xx M0 core.
# However, on GCC-ARM-Embedded 4.9 2015q2, it seems to produce non-functional
# binaries.
set(MCU cortex-m0)

# ARM-specific options here
set(AOPT)

# THUMB-specific options here
set(TOPT "-mthumb -DTHUMB")

# Define C warning options here
set(CWARN "-Wall -Wextra -Wstrict-prototypes")

# Define C++ warning options here
set(CPPWARN "-Wall -Wextra -Wno-psabi")

#
# Compiler settings
##############################################################################

##############################################################################
# Start of default section
#

# List all default C defines here, like -D_DEBUG=1
# TODO: Switch -DCRT0_INIT_DATA depending on load from RAM or SPIFI?
# NOTE: _RANDOM_TCC to kill a GCC 4.9.3 error with std::max argument types
set(DDEFS "-DLPC43XX -DLPC43XX_M0 -D__NEWLIB__ -DHACKRF_ONE -DTOOLCHAIN_GCC -DTOOLCHAIN_GCC_ARM -D_RANDOM_TCC=0")

# List all default ASM defines here, like -D_DEBUG=1
set(DADEFS)

# List all default directories to look for include files here
set(DINCDIR)

# List the default directory to look for the libraries here
set(DLIBDIR)

# List all default libraries here
set(DLIBS)

#
# End of default section
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
set(UDEFS)

# Define ASM defines here
set(UADEFS)

# List all user directories here
set(UINCDIR)

# List the user directory to look for the libraries here
set(ULIBDIR)

# List all user libraries here
set(ULIBS)

#
# End of user defines
##############################################################################

include(${CMAKE_CURRENT_SOURCE_DIR}/common/config/rules.cmake)

##############################################################################


add_executable(${PROJECT_NAME}.ppsi ${CSRC} ${CPPSRC} ${ASMSRC})
set_target_properties(${PROJECT_NAME}.ppsi PROPERTIES LINK_DEPENDS ${LDSCRIPT})
add_definitions(${DEFS})
include_directories(. ${INCDIR})
link_directories(${LLIBDIR})

#target_compile_definitions(${PROJECT_NAME}.ppsi PRIVATE "${DDEFS}")
#target_compile_features   (${PROJECT_NAME}.ppsi PRIVATE cxx_std_17)
#target_compile_options    (${PROJECT_NAME}.ppsi PRIVATE -Os -g -mcpu=cortex-m0 -mno-thumb-interwork -mthumb -fno-common --specs=nano.specs --specs=nosys.specs -fno-rtti -fno-exceptions -Weffc++ -Wuninitialized -fno-use-cxa-atexit)

target_link_libraries(${PROJECT_NAME}.ppsi -Wl,-Map=${PROJECT_NAME}.map)
target_link_libraries(${PROJECT_NAME}.ppsi "-Wl,--print-memory-usage")
#target_link_libraries(${PROJECT_NAME}.ppsi "-nostartfiles")
#target_link_libraries(${PROJECT_NAME}.ppsi "-Wl,--cref,--no-warn-mismatch")
#target_link_libraries(${PROJECT_NAME}.ppsi "-Wl,--entry=_standalone_application_information")
#target_link_libraries(${PROJECT_NAME}.ppsi "-Wl,-T${LDSCRIPT}")

# redirect std lib memory allocations
target_link_libraries(${PROJECT_NAME}.ppsi "-Wl,-wrap,_malloc_r")
target_link_libraries(${PROJECT_NAME}.ppsi "-Wl,-wrap,_free_r")

add_custom_command(
	OUTPUT ${PROJECT_NAME}.ppmp
	COMMAND ${CMAKE_OBJCOPY} -v -O binary ${PROJECT_NAME}.ppsi.elf ${PROJECT_NAME}.ppmp
	COMMAND ${CMAKE_OBJDUMP} --source ${PROJECT_NAME}.ppsi.elf > ${PROJECT_NAME}.objdump.txt
	COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/common/config/create_header.py ${PROJECT_NAME}.ppmp ${PROJECT_NAME}.h
	DEPENDS ${PROJECT_NAME}.ppsi
)

add_custom_target(
	${PROJECT_NAME} ALL
	DEPENDS ${PROJECT_NAME}.ppmp
)
