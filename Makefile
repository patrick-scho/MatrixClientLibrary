CC=clang

C_OPTS=-Wall -Wextra -pedantic
C_OPTS+=src/matrix.c
C_OPTS+=src/matrix_http_mongoose.c
C_OPTS+=ext/mjson/src/mjson.c
C_OPTS+=ext/mongoose/mongoose.c
C_OPTS+=-I src/
C_OPTS+=-I ext/olm/include/
C_OPTS+=-I ext/mjson/src/
C_OPTS+=-I ext/mongoose/
C_OPTS+=-l ws2_32
C_OPTS+=-l ssl
C_OPTS+=-l crypto
C_OPTS+=-l stdc++
C_OPTS+=out/olm/libolm.a
C_OPTS+=-D MG_ENABLE_OPENSSL=1
C_OPTS+=-fuse-ld=lld.exe -g -gcodeview -Wl,/debug,/pdb:out/test.pdb
# C_OPTS+=-I ext/curl/include/
# C_OPTS+=-L ext/curl/build/lib/
# C_OPTS+=-l curl

#C_OPTS+=-Wl,--verbose

out/examples/%: examples/%.c src/*
	$(CC) -o out/examples/$* examples/$*.c $(C_OPTS)

.PHONY: examples

examples: out/examples/Login out/examples/Send out/examples/SendEncrypted out/examples/Sync

out/olm/libolm.a:
	cd out/olm
	cmake -DBUILD_SHARED_LIBS=OFF -DOLM_TESTS=OFF ../../ext/olm
	cmake --build .