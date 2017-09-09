SRC = src/ae.c src/anet.c src/zmalloc.c
OBJ = ${SRC:.c=.o}
INCDIRS=-I/Users/liangkui/Projects/c/liae-test/c_library_v2
CFLAGS = -Wno-parentheses -Wno-switch-enum -Wno-unused-value ${INCDIRS}

libae.a: $(OBJ)
	$(AR) -rc $@ $(OBJ)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

timer: example/timer.o libae.a
	$(CC) $^ -o $@

echo: example/echo.o libae.a
	$(CC) $^ -o $@

stress: example/stress_test.o libae.a
	$(CC) $^ -o $@

udp_test: example/udp_test.o libae.a
	$(CC) $^ -o $@

clean:
	rm -f $(OBJ) libae.a example/timer.o timer example/echo.o echo example/stress_test.o stress example/udp_test.o udp_test

.PHONY: clean
