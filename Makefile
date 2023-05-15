CC=gcc

C_OPTS=-Wall -Wextra -pedantic
C_OPTS+=-I src/
C_OPTS+=-I ext/olm/include/
C_OPTS+=src/matrix.c

out/examples/%: examples/%.c
	$(CC) -o out/examples/$* examples/$*.c $(C_OPTS)


.PHONY: examples

examples: out/examples/Login out/examples/Send out/examples/SendEncrypted out/examples/Sync