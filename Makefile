config=release
LIB_NAME?=yasio
SHARE_NAME?=lib$(LIB_NAME).so
#-Werror
CXXFLAGS=-g -Wall -Wno-unused-result -Wextra -Wundef -Wcast-align -Wcast-qual -Wno-old-style-cast -Wdouble-promotion -std=$(cxxstd) -I.

ifeq ($(CXX),clang++)
	CXXFLAGS+=-Wno-gnu-zero-variadic-macro-arguments -Wno-zero-length-array -pedantic -Wshadow
endif

ifeq ($(config),release)
	CXXFLAGS+=-O3 -DNDEBUG
else
	CXXFLAGS+=-g
endif

all:$(STATIC_NAME) $(SHARE_NAME)

%.o:%.c
	$(CXX) $(CXXFLAGS) $< -o $@

SOURCE := yasio/xxsocket.cpp yasio/yasio.cpp yasio/ibstream.cpp yasio/obstream.cpp yasio/impl/yasio-ni.cpp
OBJS   := $(patsubst %.cpp,%.o,$(SOURCE))

$(STATIC_NAME):$(OBJS)
	$(AR) -r $(STATIC_NAME)

$(SHARE_NAME):$(OBJS)
	$(CXX) $(CXXFLAGS) -shared -fpic -o $(SHARE_NAME) $(SOURCE)

clean:
	rm -rf $(OBJS) $(STATIC_NAME) $(SHARE_NAME)
