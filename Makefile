.DEFAULT_GOAL := test
ICK ?=
LUA_SOURCE ?= .lua-source
SHADER_SOURCE ?= .shader-source
OUT := out/host
ICK_FLAGS := -fno-link-libatomic
CFLAGS := -std=c17 -O2 -g -Wall -Wextra -Werror
CPPFLAGS := -Iinclude -Isupport
LDLIBS := -lEGL -lGLESv2 -lm -ldl
LUA_REVISION := 87306483cec50f8c750a22dda1d0742246fad756
SHADER_REVISION := bdf7e48d0161965269354b0aa26a2befe8e16885

.PHONY: preflight test test-c test-application mutants clean
preflight:
	test -n "$(ICK)" && test -x "$(ICK)"
	test "$$(git -C "$(LUA_SOURCE)" rev-parse HEAD)" = "$(LUA_REVISION)"
	git -C "$(LUA_SOURCE)" diff --quiet HEAD --
	test "$$(git -C "$(SHADER_SOURCE)" rev-parse HEAD)" = "$(SHADER_REVISION)"
	git -C "$(SHADER_SOURCE)" diff --quiet HEAD --
	test "$$(git hash-object "$(SHADER_SOURCE)/generated/fullscreen.vert")" = a82ed3e72b842171a731d9c203f73a6989757c87
	test "$$(git hash-object "$(SHADER_SOURCE)/generated/dot-vector32-covector32.frag")" = 5b891e5c67dcf49f9bf2d6b523208be0c30552d0
	test "$$(git hash-object "$(SHADER_SOURCE)/generated/rotate-difference8-to-e1.frag")" = 49af343de226e7e1276a8b1d7110fc07d39698b3
	mkdir -p $(OUT)
	"$(ICK)" $(ICK_FLAGS) $(CFLAGS) ci/literal_probe.c -o $(OUT)/literal_probe
	$(OUT)/literal_probe
	! cc -std=c17 -c ci/literal_probe.c -o $(OUT)/stock_probe.o > $(OUT)/stock-compiler-refusal.log 2>&1
	test ! -e $(OUT)/stock_probe.o
	sha256sum "$(ICK)" "$$($(ICK) -print-prog-name=cc1)" > $(OUT)/compiler.sha256
	git rev-parse HEAD > $(OUT)/source-revision.txt
	printf 'runtime\t%s\nmathematics\t%s\n' $(LUA_REVISION) $(SHADER_REVISION) > $(OUT)/dependencies.tsv
	sha256sum "$(SHADER_SOURCE)/generated/fullscreen.vert" "$(SHADER_SOURCE)/generated/dot-vector32-covector32.frag" "$(SHADER_SOURCE)/generated/rotate-difference8-to-e1.frag" > $(OUT)/shared-shaders.sha256

$(OUT)/photo_math.o: src/photo_math.c include/photo_math.h | preflight
	"$(ICK)" $(ICK_FLAGS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/libphoto_math.a: $(OUT)/photo_math.o
	ar rcs $@ $^
$(OUT)/test_core: tests/test_core.c support/host_context.h $(OUT)/libphoto_math.a
	"$(ICK)" $(ICK_FLAGS) $(CPPFLAGS) $(CFLAGS) $< $(OUT)/libphoto_math.a $(LDLIBS) -o $@

test-c: $(OUT)/test_core
	$(OUT)/test_core "$(SHADER_SOURCE)" > $(OUT)/core.log 2>&1 || { cat $(OUT)/core.log; exit 1; }
	cat $(OUT)/core.log
	sha256sum src/photo_math.c include/photo_math.h $(OUT)/libphoto_math.a $(OUT)/test_core > $(OUT)/c-checkpoint.sha256
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
	$(OUT)/photo_ilua "$(SHADER_SOURCE)" "$(LUA_SOURCE)/testes/symbolic-assignment.lua"
	$(OUT)/photo_ilua "$(SHADER_SOURCE)" tests/policy.ilua app/classify.ilua included
	sha256sum $(OUT)/photo_ilua $(OUT)/libphoto_math.a > $(OUT)/before-policy-edit.sha256
	sed 's/policy.threshold ← 0/policy.threshold ← 15/' app/classify.ilua > $(OUT)/alternate.ilua
	$(OUT)/photo_ilua "$(SHADER_SOURCE)" tests/policy.ilua $(OUT)/alternate.ilua excluded
	sha256sum -c $(OUT)/before-policy-edit.sha256
	readelf -d $(OUT)/photo_ilua > $(OUT)/runtime-libraries.txt
	! grep -Ei 'NEEDED.*lua|NEEDED.*libstdc\+\+' $(OUT)/runtime-libraries.txt
	grep -F '$(OUT)/lua/liblua.a' $(OUT)/photo_ilua.map > $(OUT)/runtime-link-map.txt
	sha256sum $(OUT)/lua/liblua.a $(OUT)/photo_ilua "$(LUA_SOURCE)/lua.h" "$(LUA_SOURCE)/llex.c" "$(LUA_SOURCE)/lparser.c" > $(OUT)/runtime.sha256

mutants: $(OUT)/test_core
	mkdir -p $(OUT)/mutant/generated
	cp "$(SHADER_SOURCE)/generated/fullscreen.vert" "$(SHADER_SOURCE)/generated/rotate-difference8-to-e1.frag" $(OUT)/mutant/generated/
	sed 's/_idris_fragColor = _idris_t15;/_idris_fragColor = vec4(-_idris_t15.x, _idris_t15.yzw);/' "$(SHADER_SOURCE)/generated/dot-vector32-covector32.frag" > $(OUT)/mutant/generated/dot-vector32-covector32.frag
	! $(OUT)/test_core $(OUT)/mutant > $(OUT)/wrong-shared-projection.log 2>&1
	grep -F 'score == -4.0f' $(OUT)/wrong-shared-projection.log
	rm $(OUT)/mutant/generated/rotate-difference8-to-e1.frag
	! $(OUT)/test_core $(OUT)/mutant > $(OUT)/missing-shared-backend.log 2>&1
	grep -F 'shared shader missing or oversized' $(OUT)/missing-shared-backend.log
	test -z "$$(nm $(OUT)/libphoto_math.a | grep -E 'photo_svm|psvm_|svm_fit|svm_predict' || true)"

test:
	$(MAKE) test-c
	$(MAKE) mutants
	$(MAKE) test-application
clean:
	rm -rf out
