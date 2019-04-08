cxxstd=c++11
config=release
LIB_NAME?= yasio
STATIC_NAME ?= lib$(LIB_NAME).a
SHARE_NAME  ?= lib$(LIB_NAME).so

CXXFLAGS=-g -Wall -Wextra -Werror -pedantic -Wundef -Wshadow -Wcast-align -Wcast-qual -Wold-style-cast -Wdouble-promotion

ifeq ($(config),release)
	CXXFLAGS+=-O3 -DNDEBUG
endif

all:$(STATIC_NAME) $(SHARE_NAME)

%.o:%.c
    $(CXX) $(CXXFLAGS) -c $< -o $@

SOURCE := $(wildcard yasio/*.cpp)
OBJS   := $(patsubst yasio/%.cpp,%.o,$(SOURCE))

$(STATIC_NAME):$(OBJS)
    $(AR) -cr  $(STATIC_NAME)

$(SHARE_NAME):$(OBJS)
    $(CXX) $(CXXFLAGS) -shared -fpic -o $(SHARE_NAME) $(SOURCE)

clean:
    rm -rf $(OBJS) $(STATIC_NAME) $(SHARE_NAME)
