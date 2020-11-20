CC=gcc
CFLAGS=-fsanitize=signed-integer-overflow -fsanitize=undefined -g -std=gnu99 -O2 -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wshadow

sma: a3_test.c sma.c
	$(CC) -o sma.exe $(CFLAGS) a3_test.c sma.c

test: my_test.c
	${CC} my_test.c -o test.exe

clean:
	rm *.exe
