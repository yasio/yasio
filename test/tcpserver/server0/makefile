#########################################################
# xxpie_server v1.0.0                                   #
# Copyright (c) 2012 by X.D. Guo, ALL RIGHTS RESERVED.  #
#########################################################

#* --- define variables --------------------------*#
CC      = gcc
CXX     = g++
CFLAGS  = -std=c++0x -Wall -D_USING_LIBXML2 -c 
ifdef release 
TARGET  = xxpie_server_release
CFLAGS += -Os
else
TARGET  = xxpie_server_debug
CFLAGS += -g -D_DEBUG
endif
INCLUDES = -I$(XXPIESVR_ROOT)/3rdlibs/include/ -I$(XXPIESVR_ROOT)/3rdlibs/include/libxml2/ -I$(XXPIESVR_ROOT)/3rdlibs/include/mysql/ 
LIBS    = -L$(XXPIESVR_ROOT)/3rdlibs/lib/ \
          -lsimplepp -lpthread \
          -lboost_regex \
          -lboost_thread \
          -lboost_date_time \
          -lboost_system \
          -lboost_serialization \
          -lboost_chrono \
          -lmysql \
          -lxml2 
OBJDIR  = obj
#*** widcard usage: SOURCES = $(wildcard %.cpp)
C_SOURCES = $(wildcard *.c)
CPP_SOURCES += $(wildcard *.cpp)

#SOURCES = xxsocket.cpp \
#          thread_basic.cpp \
#          thread_synch.cpp \
#          threadpool.cpp
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
	@if [ ! -d $(OBJDIR)  ];then mkdir $(OBJDIR) ;fi 
	@$(CXX) $(CFLAGS) $< -o $@ $(INCLUDES)
	@echo "\033[49;32;1mcomplete.\033[0m"

$(OBJDIR)/%.o: %.c
	@echo -n "\033[49;33;1mcompiling\033[0m" $<...
	@if [ ! -d $(OBJDIR)  ];then mkdir $(OBJDIR) ;fi 
	@$(CXX) $(CFLAGS) $< -o $@ $(INCLUDES)
	@echo "\033[49;32;1mcomplete.\033[0m"


#*--- phony targets -------------------------------*#
install:
	@echo install...
	@mkdir -p /opt/xxpie_server/
	@cp -f $(TARGET) /opt/xxpie_server/
	@cp -f scripts/xxpie_svr /opt/xxpie_server/
	@echo install xxpie_server sucessfully.
clean:
	@echo clean all...
	@rm $(TARGET) $(OBJECTS) -f 

all: $(TARGET)
	

