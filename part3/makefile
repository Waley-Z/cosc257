LLVMCODE = llvm_parser
TEST_FILES = cfold_add.ll cfold_mul.ll cfold_sub.ll p2_common_subexpr.ll
# TEST_FILES = p3_const_prop.ll
OPTIMIZED_FILES = $(patsubst %.ll, %_opt.ll, $(TEST_FILES))
TEST_DIR = optimizer_test_results

$(LLVMCODE): $(LLVMCODE).cpp
	g++ -g -I /usr/include/llvm-c-15/ -c $(LLVMCODE).cpp
	g++ $(LLVMCODE).o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -o $@

llvm_file: $(TEST).c
	clang-15 -S -emit-llvm $(TEST).c -o $(TEST).ll

run: $(LLVMCODE) $(addprefix $(TEST_DIR)/, $(TEST_FILES))
	@for test_file in $(TEST_FILES); do \
		./$(LLVMCODE) $(TEST_DIR)/$$test_file; \
		tail -n +2 test_new.ll > test_new_stripped.ll; \
		tail -n +2 $(TEST_DIR)/$${test_file%.*}_opt.ll > $${test_file%.*}_opt_stripped.ll; \
		if diff -q test_new_stripped.ll $${test_file%.*}_opt_stripped.ll >/dev/null; then \
			echo "$$test_file passed\n"; \
		else \
			echo "$$test_file failed\n"; \
			diff -u test_new.ll $(TEST_DIR)/$${test_file%.*}_opt.ll; \
		fi; \
		rm -f test_new_stripped.ll $${test_file%.*}_opt_stripped.ll; \
	done

clean: 
	rm -rf $(TEST).ll
	rm -rf $(LLVMCODE)
	rm -rf *.o
	rm -rf test_new.ll
