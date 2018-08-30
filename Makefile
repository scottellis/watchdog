CC = gcc
CFLAGS = -O2 -Wall

TARGET = watchdog

$(TARGET): main.c
	$(CC) $(CFLAGS) $< -o $@


clean:
	rm -f $(TARGET) 
