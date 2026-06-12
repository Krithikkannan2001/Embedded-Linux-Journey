CC = gcc
CFLAGS = -Wall -g

TARGET = calculator
OBJS = main.o math_ops.o

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

main.o: main.c math_ops.h
	$(CC) $(CFLAGS) -c main.c

math_ops.o: math_ops.c math_ops.h
	$(CC) $(CFLAGS) -c math_ops.c

clean:
	rm -f $(OBJS) $(TARGET)
