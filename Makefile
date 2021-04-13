CC=gcc

CFLAGS=-I. -Wall
LIBS=-lpthread -lstdc++
SOURCE=$(wildcard *.cc)
HDRS=$(wildcard *.h)
TARGET=sim16f
OBJS=$(SOURCE:.cc=.o)
DEPENDS=$(SOURCE:.cc=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LFLAGS) $(LIBS)

-include $(DEPENDS)

%o: %.cc Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -f *.o *.d $(TARGET)
