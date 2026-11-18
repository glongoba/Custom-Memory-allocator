#compiler
CC = clang
#flags
CFLAGS = -Wall -Wextra -I. -g

TARGET= test.exe

VALGRIND_FLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes

SRCS = mymemory.c \
	   main.c 

OBJS = $(SRCS:.c=.o)

all:  $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@

valgrind: $(TARGET)
	valgrind $(VALGRIND_FLAGS) ./$(TARGET)

clean:
	rm -rf $(OBJS) $(TARGET)