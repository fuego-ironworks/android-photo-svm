# Consume the ICK compiler from the qualified ai-ci/ick-android action.
# Run with -C android -f Makefile -f ci/native.mk native-build.
NATIVE := $(REGIONS) native/photo_jni.c
ICK_REVISION := fbe86e23d55cfec2000c08e61deea2a407fd7175
ICK_SOURCE ?= ../.ick-android-$(ABI)-source
NDK_REVISION := 29.0.14206865
ANDROID_API := 23
GNU_arm64-v8a := aarch64-linux-gnu
GNU_armeabi-v7a := arm-linux-gnueabi
GNU_x86_64 := x86_64-linux-gnu
NDK_arm64-v8a := aarch64-linux-android
NDK_armeabi-v7a := armv7a-linux-androideabi
NDK_x86_64 := x86_64-linux-android
HEADER_arm64-v8a := aarch64-linux-android
HEADER_armeabi-v7a := arm-linux-androideabi
HEADER_x86_64 := x86_64-linux-android
MACHINE_arm64-v8a := AArch64
MACHINE_armeabi-v7a := ARM
MACHINE_x86_64 := Advanced Micro Devices X86-64
PLATFORM := $(NDK)/toolchains/llvm/prebuilt/linux-x86_64
SYSROOT := $(PLATFORM)/sysroot
LINKER := $(PLATFORM)/bin/$(NDK_$(ABI))$(ANDROID_API)-clang
NM := $(PLATFORM)/bin/llvm-nm
READELF := $(PLATFORM)/bin/llvm-readelf
NATIVE_FLAGS := $(TARGET_FLAGS) --sysroot="$(SYSROOT)" -nostdinc \
	-isystem "$(ICK_BUILTIN_INCLUDE)" -isystem "$(SYSROOT)/usr/include" \
	-isystem "$(SYSROOT)/usr/include/$(HEADER_TARGET)" -D__ANDROID__ \
	-D__ANDROID_API__=$(ANDROID_API) -D__ANDROID_MIN_SDK_VERSION__=$(ANDROID_API) \
	-DBIONIC_IOCTL_NO_SIGNEDNESS_OVERLOAD -std=c11 -O2 -Wall -Wextra -Werror \
	-pedantic -fPIC -fvisibility=hidden -fno-stack-protector -fno-fast-math -ffp-contract=off
NATIVE_OBJECTS := $(patsubst native/%.c,$(OUT)/objects/%.o,$(NATIVE))

.PHONY: native-build native-preflight
.SECONDARY:
native-preflight:
	test -n "$(GNU_$(ABI))"
	test -n "$(REGIONS)"
	test "$(GNU_TARGET)" = "$(GNU_$(ABI))"
	test "$(HEADER_TARGET)" = "$(HEADER_$(ABI))"
	test "$$(git rev-parse HEAD)" = "$(SOURCE_REVISION)"
	test "$$(git -C "$(ICK_SOURCE)" rev-parse HEAD)" = "$(ICK_REVISION)"
	test "$$(realpath "$(ICK)")" = "$$(realpath "$(ICK_STAGE)/bin/$(GNU_TARGET)-gcc")"
	test "$$("$(ICK)" -dumpmachine)" = "$(GNU_TARGET)"
	test "$$("$(ICK)" -print-file-name=include)" = "$(ICK_BUILTIN_INCLUDE)"
	test -x "$$("$(ICK)" -print-prog-name=cc1)"
	test -x "$(LINKER)" && test -x "$(NM)" && test -x "$(READELF)"
	test -s "$(SYSROOT)/usr/include/jni.h"
	grep -Ex 'Pkg[.]Revision[[:space:]]*=[[:space:]]*$(subst .,[.],$(NDK_REVISION))' "$(NDK)/source.properties"
	test "$(OUT)" != "" && test "$(abspath $(OUT))" = "$(OUT)" && test ! -e "$(OUT)"
	mkdir -p "$(OUT)/objects"

$(OUT)/objects/%.s: native/%.c | native-preflight
	"$(ICK)" $(NATIVE_FLAGS) -S "$<" -o "$@"

$(OUT)/objects/%.o: $(OUT)/objects/%.s
	"$(LINKER)" $(TARGET_FLAGS) -c "$<" -o "$@"

native-build: native-preflight $(NATIVE_OBJECTS)
	"$(NM)" --undefined-only $(NATIVE_OBJECTS) > "$(OUT)/object-imports.txt"
	awk '/__atomic_|__sync_|__stack_chk/ { print > "/dev/stderr"; bad=1 } END { exit bad }' "$(OUT)/object-imports.txt"
	"$(LINKER)" -shared -Wl,-z,defs -Wl,-z,relro -Wl,-z,now \
		-Wl,-z,max-page-size=16384 -Wl,-soname,libphotosvm.so \
		$(NATIVE_OBJECTS) -o "$(OUT)/libphotosvm.so"
	"$(NM)" -D --defined-only --format=posix "$(OUT)/libphotosvm.so" > "$(OUT)/exports.txt"
	awk '$$1 ~ /^Java_/ { print $$1 }' "$(OUT)/exports.txt" | LC_ALL=C sort > "$(OUT)/observed-jni.txt"
	awk '$$1 ~ /^C:/ { print "Java_org_isomorphisms_photosvm_PhotoActivity_" $$2 }' \
		dex/photo-jni.tsv | LC_ALL=C sort > "$(OUT)/expected-jni.txt"
	test -s "$(OUT)/expected-jni.txt"
	diff -u "$(OUT)/expected-jni.txt" "$(OUT)/observed-jni.txt"
	"$(READELF)" -h -lW -d "$(OUT)/libphotosvm.so" > "$(OUT)/elf.txt"
	grep -Ex ' *Machine: +$(MACHINE_$(ABI))' "$(OUT)/elf.txt"
	grep -E '\(SONAME\).*\[libphotosvm[.]so\]' "$(OUT)/elf.txt"
	awk '$$1=="LOAD" { n++; if ($$NF!="0x4000") bad=1 } END { exit !(n && !bad) }' "$(OUT)/elf.txt"
	sha256sum $(NATIVE) native/*.h dex/photo-jni.tsv > "$(OUT)/source.sha256"
	sha256sum "$(ICK)" "$$("$(ICK)" -print-prog-name=cc1)" > "$(OUT)/compiler.sha256"
	sha256sum "$(NDK)/source.properties" "$(SYSROOT)/usr/include/jni.h" \
		"$(LINKER)" "$(PLATFORM)/bin/clang" "$(PLATFORM)/bin/ld.lld" > "$(OUT)/ndk.sha256"
	cd "$(OUT)" && sha256sum objects/*.s objects/*.o libphotosvm.so > artifacts.sha256
	printf 'ick_native_source\tPASS\nABI\t%s\nAndroid_API\t%s\nsource_revision\t%s\nICK_revision\t%s\nNDK_revision\t%s\ninstalled_ART\tNOT_RUN\nphysical_C67\tNOT_RUN\n' \
		"$(ABI)" "$(ANDROID_API)" "$(SOURCE_REVISION)" "$(ICK_REVISION)" "$(NDK_REVISION)" > "$(OUT)/result.tsv"
