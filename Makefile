config=release
TARGET=libyasio.so

CXXFLAGS = -c -fPIC -Wall -Wno-unused-result -Wextra -Wundef -Wcast-align -Wcast-qual -Wno-old-style-cast -Wdouble-promotion -DYASIO_HAVE_KCP=1 -std=$(cxxstd)
CFLAGS = -c -fPIC

ifeq ($(CXX),clang++)
	CXXFLAGS+=-Wno-gnu-zero-variadic-macro-arguments -Wno-zero-length-array -pedantic -Wshadow
endif

ifeq ($(config),release)
	CXXFLAGS+=-O3 -DNDEBUG
else
	CXXFLAGS+=-g
endif

LIBS = -lpthread

INCLUDES = -I.
 
CXXFILES = yasio/ibstream.cpp yasio/obstream.cpp yasio/xxsocket.cpp yasio/yasio.cpp yasio/bindings/yasio_ni.cpp
CFILES = yasio/kcp/ikcp.c
 
OBJFILE = $(CFILES:.c=.o) $(CXXFILES:.cpp=.o)
 
all:$(TARGET)
 
$(TARGET): $(OBJFILE)
	$(CXX) $^ $(LIBS) -fPIC -shared -o $@
 
%.o:%.c
	$(CC) -o $@ $(CFLAGS) $< $(INCLUDES)
 
%.o:%.cpp
	$(CXX) -o $@ $(CXXFLAGS) $< $(INCLUDES)

clean:
	rm -rf $(TARGET) $(OBJFILE)
