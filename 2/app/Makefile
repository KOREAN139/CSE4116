CC = arm-none-linux-gnueabi-gcc
CFLAGS = -static
TARGET = app
OBJECTS = huins_app.o

all : $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean :
	rm *.o app
