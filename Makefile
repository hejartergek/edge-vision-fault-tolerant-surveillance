CC = gcc

CFLAGS = -Wall -Wextra -g3

INCLUDES = -Iinclude

TARGET = build/edge_vision

C_OBJECTS = \
	build/main.o \
	build/frame_queue.o \
	build/image_processor.o \
	build/synthetic_camera.o

all: $(TARGET)

$(TARGET): $(C_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

build/main.o: src/main.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/frame_queue.o: src/frame_queue.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/image_processor.o: src/image_processor.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

build/synthetic_camera.o: src/synthetic_camera.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f build/*.o
	rm -f $(TARGET)

.PHONY: all clean