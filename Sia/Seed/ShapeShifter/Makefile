# Project Name
TARGET = ShapeShifter

#include my DSP library
DSP_Lib= ../../lib/
include $(DSP_Lib)/Makefile

# Sources
CPP_SOURCES = ShapeShifter.cpp ShapeShifterHW.cpp applet.cpp

#include every new folder in the library here.
C_INCLUDES += -I/UI/
C_INCLUDES += -I/Applets/

# Library Locations
LIBDAISY_DIR = ../../../libDaisy/
DAISYSP_DIR = ../../../DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
