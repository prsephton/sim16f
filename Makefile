CC=gcc
CPP=gcc

GTKCFLAGS=$(shell pkg-config --cflags gtk+-3.0 gtkmm-3.0)
GTKLFLAGS=$(shell pkg-config --libs gtk+-3.0 gtkmm-3.0)
CFLAGS=-I. -Wall $(GTKCFLAGS)
LFLAGS=$(GTKLFLAGS)
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

%.o: %.cc
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -f *.o *.d $(TARGET)
