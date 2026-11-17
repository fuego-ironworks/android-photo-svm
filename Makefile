.DEFAULT_GOAL := test
ICK ?=
LUA_SOURCE ?= .lua-source
SHADER_SOURCE ?= .shader-source
SHADER_BUNDLE ?= $(SHADER_SOURCE)/build/photo-math
OUT := out/host
ICK_FLAGS := -fno-link-libatomic
CFLAGS := -std=c17 -O2 -g -Wall -Wextra -Werror
CPPFLAGS := -Iinclude -Isupport
LDLIBS := -lEGL -lGLESv2 -lm -ldl
LUA_REVISION := 87306483cec50f8c750a22dda1d0742246fad756
SHADER_REVISION := d8781df201e66561d63dcf04bfb1af6d0684fc95

.PHONY: preflight test test-c test-application mutants clean
preflight:
	test -n "$(ICK)" && test -x "$(ICK)"
	test "$$(git -C "$(LUA_SOURCE)" rev-parse HEAD)" = "$(LUA_REVISION)"
	git -C "$(LUA_SOURCE)" diff --quiet HEAD --
	test "$$(git -C "$(SHADER_SOURCE)" rev-parse HEAD)" = "$(SHADER_REVISION)"
	git -C "$(SHADER_SOURCE)" diff --quiet HEAD --
	test "$$(cat "$(SHADER_BUNDLE)/source-revision.txt")" = "$(SHADER_REVISION)"
	grep -F '#define PHOTO_MATH_REVISION "$(SHADER_REVISION)"' include/photo_math.h
	cd "$(SHADER_SOURCE)" && sha256sum -c "$(abspath $(SHADER_BUNDLE))/source.sha256"
	cd "$(SHADER_BUNDLE)" && sha256sum -c generated.sha256
	mkdir -p $(OUT)
	"$(ICK)" $(ICK_FLAGS) $(CFLAGS) ci/literal_probe.c -o $(OUT)/literal_probe
	$(OUT)/literal_probe
	! cc -std=c17 -c ci/literal_probe.c -o $(OUT)/stock_probe.o > $(OUT)/stock-compiler-refusal.log 2>&1
	test ! -e $(OUT)/stock_probe.o
	sha256sum "$(ICK)" "$$($(ICK) -print-prog-name=cc1)" > $(OUT)/compiler.sha256
	git rev-parse HEAD > $(OUT)/source-revision.txt
	printf 'runtime\t%s\nmathematics\t%s\n' $(LUA_REVISION) $(SHADER_REVISION) > $(OUT)/dependencies.tsv
	cp "$(SHADER_BUNDLE)/generated.sha256" $(OUT)/shared-shaders.sha256
	cp "$(SHADER_BUNDLE)/source.sha256" $(OUT)/shared-source.sha256

$(OUT)/photo_math.o: src/photo_math.c include/photo_math.h | preflight
	"$(ICK)" $(ICK_FLAGS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/libphoto_math.a: $(OUT)/photo_math.o
	ar rcs $@ $^
$(OUT)/test_core: tests/test_core.c support/host_context.h $(OUT)/libphoto_math.a
	"$(ICK)" $(ICK_FLAGS) $(CPPFLAGS) $(CFLAGS) $< $(OUT)/libphoto_math.a $(LDLIBS) -o $@
$(OUT)/test_rotation: tests/test_rotation.c support/host_context.h $(OUT)/libphoto_math.a
	"$(ICK)" $(ICK_FLAGS) $(CPPFLAGS) $(CFLAGS) $< $(OUT)/libphoto_math.a $(LDLIBS) -o $@

test-c: $(OUT)/test_core $(OUT)/test_rotation
	$(OUT)/test_core "$(SHADER_BUNDLE)" > $(OUT)/core.log 2>&1 || { cat $(OUT)/core.log; exit 1; }
	$(OUT)/test_rotation "$(SHADER_BUNDLE)" > $(OUT)/rotation.log 2>&1 || { cat $(OUT)/rotation.log; exit 1; }
	cat $(OUT)/core.log $(OUT)/rotation.log
	sha256sum src/photo_math.c include/photo_math.h $(OUT)/libphoto_math.a $(OUT)/test_core $(OUT)/test_rotation > $(OUT)/c-checkpoint.sha256
	cp $(OUT)/source-revision.txt $(OUT)/c-checkpoint-revision.txt

$(OUT)/lua/liblua.a: | preflight
	mkdir -p $(OUT)/lua
	cp "$(LUA_SOURCE)/"*.[ch] $(OUT)/lua/
	cp "$(LUA_SOURCE)/makefile" $(OUT)/lua/makefile
	$(MAKE) -C $(OUT)/lua -f "$(abspath $(LUA_SOURCE))/makefile" CC="$(ICK) $(ICK_FLAGS)" MYCFLAGS="-std=c99 -DLUA_USE_POSIX" MYLDFLAGS= MYLIBS=-ldl liblua.a lua
$(OUT)/photo_ilua: host/ilua_main.c support/host_context.h $(OUT)/libphoto_math.a $(OUT)/lua/liblua.a
	"$(ICK)" $(ICK_FLAGS) $(CPPFLAGS) -I"$(LUA_SOURCE)" $(CFLAGS) $< $(OUT)/libphoto_math.a $(OUT)/lua/liblua.a -Wl,-Map,$@.map $(LDLIBS) -o $@

test-application: $(OUT)/photo_ilua
	sha256sum -c $(OUT)/c-checkpoint.sha256
	cmp $(OUT)/source-revision.txt $(OUT)/c-checkpoint-revision.txt
	$(OUT)/lua/lua "$(LUA_SOURCE)/testes/symbolic-assignment.lua"
	$(OUT)/photo_ilua "$(SHADER_BUNDLE)" "$(LUA_SOURCE)/testes/symbolic-assignment.lua"
	$(OUT)/photo_ilua "$(SHADER_BUNDLE)" tests/rotations.ilua app/geometry.ilua
	$(OUT)/photo_ilua "$(SHADER_BUNDLE)" tests/decisions.ilua app/decisions.ilua
	$(OUT)/photo_ilua "$(SHADER_BUNDLE)" tests/policy.ilua app/classify.ilua included
	sha256sum $(OUT)/photo_ilua $(OUT)/libphoto_math.a > $(OUT)/before-policy-edit.sha256
	sed 's/policy.threshold ← 0/policy.threshold ← 15/' app/classify.ilua > $(OUT)/alternate.ilua
	$(OUT)/photo_ilua "$(SHADER_BUNDLE)" tests/policy.ilua $(OUT)/alternate.ilua excluded
	sha256sum -c $(OUT)/before-policy-edit.sha256
	readelf -d $(OUT)/photo_ilua > $(OUT)/runtime-libraries.txt
	! grep -Ei 'NEEDED.*lua|NEEDED.*libstdc\+\+' $(OUT)/runtime-libraries.txt
	grep -F '$(OUT)/lua/liblua.a' $(OUT)/photo_ilua.map > $(OUT)/runtime-link-map.txt
	sha256sum $(OUT)/lua/liblua.a $(OUT)/photo_ilua "$(LUA_SOURCE)/lua.h" "$(LUA_SOURCE)/llex.c" "$(LUA_SOURCE)/lparser.c" > $(OUT)/runtime.sha256

mutants: $(OUT)/test_core $(OUT)/test_rotation
	sh tests/shared-mutants.sh "$(SHADER_BUNDLE)" "$(OUT)"

test:
	$(MAKE) test-c
	$(MAKE) mutants
	$(MAKE) test-application
clean:
	rm -rf out
