#!/bin/make
# THIS IS AN UNPUBLISHED WORK CONTAINING FANVIL TECHNOLOGY CO., LTD. 
# CONFIDENTIAL AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE 
# FOLLOWING NOTICE APPLIES: "COPYRIGHT 2014 FANVIL TECHNOLOGY CO., LTD. 
# ALL RIGHTS RESERVED"
#
# Author: Terry Rong, 2015-05-14

DIR_INC = ./include
DIR_SRC = ./src
DIR_OBJ = ./obj
DIR_BIN = ./bin

DIR_BLUEZ = ~/bt_test/install

SRC = $(wildcard $(DIR_SRC)/*.c)
OBJ = $(patsubst %.c,$(DIR_OBJ)/%.o,$(notdir $(SRC)))

INC_DIRS := . $(DIR_INC) $(DIR_BLUEZ)/include $(DIR_BLUEZ)/include/dbus-1.0 $(DIR_BLUEZ)/lib/dbus-1.0/include
DIR_LIB = $(DIR_BLUEZ)/lib
BLUEZ_LIB = -L$(DIR_LIB) -ldbus-1 -lbluetooth
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
LDFLAGS += $(BLUEZ_LIB)
LDFLAGS +=-pthread -lrt
TARGET = BLUEZ_TEST

BIN_TARGET = $(DIR_BIN)/$(TARGET)

CFLAGS = -g -Wall $(INC_FLAGS)
CC = /*/*/arm-none-linux-gnueabi-gcc

$(BIN_TARGET):$(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)
	
$(DIR_OBJ)/%.o:${DIR_SRC}/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:clean
clean:
	-rm -f *.a $(DIR_OBJ)/*.o
