ICK ?=
JAVA_HOME ?=
OUT ?= out/android-photo
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror -pedantic
REGIONS := native/photo_regions.c native/cgw_sha256.c
.PHONY: preflight test-host clean
preflight:
	@test -n "$(ICK)" && test -x "$(ICK)" || { echo 'FAIL require explicit owned ICK path; no compiler fallback' >&2; exit 1; }
	@test -s "$(JAVA_HOME)/include/jni.h" || { echo 'FAIL require explicit JDK JNI header for host adapter fixture' >&2; exit 1; }
	@mkdir -p "$(OUT)"
test-host: preflight
	"$(ICK)" $(CFLAGS) tests/test_photo_regions.c $(REGIONS) -o "$(OUT)/test_regions"
	"$(OUT)/test_regions"
	"$(ICK)" $(CFLAGS) -pthread -I"$(JAVA_HOME)/include" -I"$(JAVA_HOME)/include/linux" tests/test_photo_jni.c $(REGIONS) native/photo_jni.c -o "$(OUT)/test_jni"
	"$(OUT)/test_jni"
	sha256sum $(REGIONS) native/photo_jni.c tests/*.c "$(OUT)/test_regions" "$(OUT)/test_jni" > "$(OUT)/checkpoint.sha256"
clean:
	rm -rf "$(OUT)"
