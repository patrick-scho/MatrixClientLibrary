CC=clang

C_OPTS=-Wall -Wextra -pedantic
C_OPTS+=src/matrix.c
C_OPTS+=src/matrix_http_mongoose.c
C_OPTS+=out/*.o
C_OPTS+=-I src/
C_OPTS+=-I ext/olm/include/
C_OPTS+=-I ext/mjson/src/
C_OPTS+=-I ext/mongoose/
C_OPTS+=-l ws2_32
C_OPTS+=-l ssl
C_OPTS+=-l crypto
C_OPTS+=-l stdc++
#C_OPTS+=-fuse-ld=lld.exe -g -gcodeview -Wl,/debug,/pdb:out/test.pdb
#C_OPTS+=-Wl,--verbose

out/examples/%: examples/%.c src/*
	$(CC) -o out/examples/$* examples/$*.c $(C_OPTS)

olm:
	cd out && \
	$(CC) -c \
		../ext/olm/src/* \
		../ext/olm/lib/crypto-algorithms/aes.c \
		../ext/olm/lib/curve25519-donna/curve25519-donna.c \
		../ext/olm/lib/crypto-algorithms/sha256.c \
		-I ../ext/olm/include \
		-I ../ext/olm/lib \
		-DOLM_STATIC_DEFINE \
		-DOLMLIB_VERSION_MAJOR=3 \
		-DOLMLIB_VERSION_MINOR=2 \
		-DOLMLIB_VERSION_PATCH=15

mongoose:
	cd out && \
	$(CC) -c ../ext/mongoose/mongoose.c -I ../ext/mongoose/ -DMG_ENABLE_OPENSSL=1

mjson:
	cd out && \
	$(CC) -c ../ext/mjson/src/mjson.c -I ../ext/mjson/src/

deps: olm mongoose mjson

.PHONY: deps olm mongoose mjson