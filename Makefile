CC ?= clang
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
CPPFLAGS += -Iinclude
LDLIBS += -lm

.PHONY: test clean sanitize
build/test_photo_svm: src/photo_svm.c include/photo_svm.h tests/test_photo_svm.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) src/photo_svm.c tests/test_photo_svm.c -o $@ $(LDLIBS)

test: build/test_photo_svm
	./build/test_photo_svm

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS='-std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer' test

clean:
	rm -rf build

build/segment_ppm: src/photo_svm.c include/photo_svm.h tools/segment_ppm.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) src/photo_svm.c tools/segment_ppm.c -o $@ $(LDLIBS)

.PHONY: demo-test
demo-test: build/segment_ppm
	sh ./tests/test_cli.sh ./build/segment_ppm
