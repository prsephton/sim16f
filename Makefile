CC=gcc
CPP=gcc

GPERF=
GTKCFLAGS=$(shell pkg-config --cflags gtk+-3.0 gtkmm-3.0)
GTKLFLAGS=$(shell pkg-config --libs gtk+-3.0 gtkmm-3.0)
CFLAGS=-I. -Wall $(GTKCFLAGS)
LFLAGS=$(GTKLFLAGS)
LIBS= -lpthread -lstdc++ -lm
SOURCE=$(wildcard src/*/*.cc src/*.cc)
TSOURCE=$(wildcard test/*.cc)
HDRS=$(wildcard src/*/*.h src/*.h)
TARGET=sim16f
OBJS=$(SOURCE:.cc=.o)
TOBJS=$(TSOURCE:.cc=.o)
DEPENDS=$(SOURCE:.cc=.d) $(TSOURCE:.cc=.d)

#debug:
#	echo $(SOURCE)
#	echo $(HDRS)

.PHONY: all clean

all: $(TARGET)

profile: GPERF=-pg
profile: $(OBJS) $(HDRS)
	$(CC) $(GPERF) $(CFLAGS) -o $(TARGET) $(OBJS) $(LFLAGS) $(LIBS)


$(TARGET): TESTING_FLAGS=
$(TARGET): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LFLAGS) $(LIBS)

-include $(DEPENDS)

%.o: %.cc
	$(CC) $(GPERF) $(TESTING_FLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

test: TESTING_FLAGS = -DTESTING
test: $(TOBJS) $(OBJS) $(HDRS)
	$(CC) $(TESTING_FLAGS) $(CFLAGS) -o run_tests $(TOBJS) $(OBJS) $(LFLAGS) $(LIBS)

clean:
	rm -f $(OBJS) $(TOBJS) $(TARGET)
