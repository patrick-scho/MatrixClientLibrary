CC=gcc

C_OPTS=-Wall -Wextra -pedantic
C_OPTS+=-I src/
C_OPTS+=-I ext/olm/include/
C_OPTS+=-I ext/mjson/src
C_OPTS+=src/matrix.c
C_OPTS+=src/matrix_http_curl.c
C_OPTS+=ext/mjson/src/mjson.c
C_OPTS+=-l curl

out/examples/%: examples/%.c src/*
	$(CC) -o out/examples/$* examples/$*.c $(C_OPTS)


.PHONY: examples

examples: out/examples/Login out/examples/Send out/examples/SendEncrypted out/examples/Sync