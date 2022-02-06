CLANG_TIDY_BINARY = /usr/local/opt/llvm/bin/clang-tidy
CLANG_APPLY_REPLACEMENTS_BINARY = /usr/local/opt/llvm/bin/clang-apply-replacements
NPROCS = 8
BUILD_DIR =./build/

.PHONY: clean
clean: 
	rm -rf "$(BUILD_DIR)"


.PHONY: clang-tidy
clang-tidy: 
	cmake -DCLANG_TIDY="$(CLANG_TIDY_BINARY)" -B "$(BUILD_DIR)" ./ && \
		make -C "$(BUILD_DIR)" -j$(NPROCS)

.PHONY: clang-tidy-fix
clang-tidy-fix: 
	cmake -DCLANG_TIDY="$(CLANG_TIDY_BINARY);--fix" -B "$(BUILD_DIR)" ./ && \
		make -C "$(BUILD_DIR)" -j$(NPROCS)

.PHONY: iwyu
iwyu: 
	iwyu_tool.py -p .

.PHONY: iwyu-fix
iwyu-fix: 
	iwyu_tool.py -p . | fix_includes.py
