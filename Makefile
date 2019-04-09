config=release
LIB_NAME?=yasio
SHARE_NAME?=lib$(LIB_NAME).so
#-Werror
CXXFLAGS=-g -Wall -Wno-unused-result -Wextra -pedantic -Wundef -Wshadow -Wcast-align -Wcast-qual -Wno-old-style-cast -Wdouble-promotion -std=c++11 -I.

ifneq ($(CXX),gcc)
	CXXFLAGS+=-Wno-gnu-zero-variadic-macro-arguments -Wno-zero-length-array
endif

ifeq ($(config),release)
	CXXFLAGS+=-O3 -DNDEBUG
else
	CXXFLAGS+=-g
endif

all:$(STATIC_NAME) $(SHARE_NAME)

%.o:%.c
	$(CXX) $(CXXFLAGS) $< -o $@

SOURCE := yasio/xxsocket.cpp yasio/yasio.cpp yasio/ibstream.cpp yasio/obstream.cpp
OBJS   := $(patsubst %.cpp,%.o,$(SOURCE))

$(STATIC_NAME):$(OBJS)
	$(AR) -r $(STATIC_NAME)

$(SHARE_NAME):$(OBJS)
	$(CXX) $(CXXFLAGS) -shared -fpic -o $(SHARE_NAME) $(SOURCE)

clean:
	rm -rf $(OBJS) $(STATIC_NAME) $(SHARE_NAME)
