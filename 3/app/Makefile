CC = arm-none-linux-gnueabi-gcc
CFLAGS = -static
TARGET = app
OBJECTS = stopwatch_app.o

all : $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean :
	rm *.o app
