#########################################################
# httpserver v1.0.0                                     #
# Copyright (c) 2014 by X.D. Guo, ALL RIGHTS RESERVED.  #
#########################################################

#* --- define variables --------------------------*#
CC      = gcc
CXX     = g++
CFLAGS  = -std=c++0x -Wall -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_SYSTEM_NO_DEPRECATED 
ifdef release 
TARGET  = tinyflower_server_r
CFLAGS += -Os
else
TARGET  = tinyflower_server_d
CFLAGS += -g -D_DEBUG
endif
INCLUDES = -I$(SVR_ROOT)/3rdlibs/include/ -I$(SVR_ROOT)/3rdlibs/include/mysql/ 
LIBS    = -L$(SVR_ROOT)/3rdlibs/lib/ \
          -lpthread \
          -ldl \
          -lssl \
          -lcrypto \
          -lmysqlclient
OBJDIR  = obj
#*** widcard usage: SOURCES = $(wildcard %.cpp)
C_SOURCES = $(wildcard *.c)
CPP_SOURCES += $(wildcard *.cpp)

C_SOURCES += thelib/utils/aes_core.c \
             thelib/utils/aes_modes.c \
             thelib/utils/jxcodec.c \
             thelib/utils/libb64.c \
             thelib/utils/md5.c \
             thelib/utils/oslib.c

CPP_SOURCES += thelib/utils/aes_wrapper.cpp \
               thelib/utils/cjson.cpp \
               thelib/utils/cjson_wrapper.cpp \
               thelib/utils/crypto_wrapper.cpp \
               thelib/utils/xxfsutility.cpp \
               thelib/utils/thread_synch.cpp \
               thelib/utils/mysqlcc.cpp \
               thelib/utils/xxsocket.cpp

#*** patsubst: <pattern>, <replacement>, <text>
OBJECTS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(CPP_SOURCES)) $(patsubst %.c, $(OBJDIR)/%.o, $(C_SOURCES))

#*--- specific file serach path ------------------*#
#vpath xsocket.cpp lib

#*--- specific phony targets ---------------------*#
.PHONY: install clean all 

#*--- rules --------------------------------------*#
#*** $^: all dependency, whole
#*** $<: all dependency, one by one
#*** $@: all targets, one by one0
$(TARGET): $(OBJECTS)
	@echo -n "\033[49;33;1mlinking\033[0m" $(TARGET)...
	@$(CXX) $^ -o $@ $(LIBS)
	@echo "\033[49;32;1mcomplete.\033[0m" 
	@echo "\033[49;32;1mcongrantulations, all of your building are sucessfully.\033[0m"

$(OBJDIR)/%.o: %.cpp
	@echo -n "\033[49;33;1mcompiling\033[0m" $<...
	@if [ ! -d $(dir $@)  ];then mkdir -p $(dir $@) ;fi 
	@$(CXX) $(CFLAGS) $< -o $@ -c $(INCLUDES) >>compile.log 2>&1
	@echo "\033[49;32;1mcomplete.\033[0m"

$(OBJDIR)/%.o: %.c
	@echo -n "\033[49;33;1mcompiling\033[0m" $<...
	@if [ ! -d $(dir $@)  ];then mkdir -p $(dir $@) ;fi 
	@$(CC) $(CFLAGS) $< -o $@ -c $(INCLUDES) >>compile.log 2>&1
	@echo "\033[49;32;1mcomplete.\033[0m"


#*--- phony targets -------------------------------*#
clean:
	@echo clean all...
	rm compile.log -f
	rm $(TARGET) $(OBJECTS) -f 

all: $(TARGET)
	

