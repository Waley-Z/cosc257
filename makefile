# Compiler
CXX = g++
CXXFLAGS = -Wall -g

# Source files
SOURCES = main.cpp \
          part1/semantic.cpp part1/lex.yy.c part1/y.tab.c part1/ast.cpp \
		  part2/ir_builder.cpp \
          part3/llvm_parser.cpp \
		  part4/assembly_generator.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Object files that require LLVM_LDFLAGS
LLVM_OBJECTS = main.o part2/ir_builder.o part3/llvm_parser.o part4/assembly_generator.o

# Executable
EXECUTABLE = main
TEST_C = part3/optimizer_test_results/p5_const_prop.c
TEST_LL = $(TEST_C:.c=.ll)

# Libraries
LLVM_LDFLAGS = `llvm-config-15 --cxxflags --ldflags --libs core`
LLVM_INCLUDE = -I /usr/include/llvm-c-15/

# Targets
all: $(EXECUTABLE)

run: $(EXECUTABLE) $(TEST_C) $(TEST_LL)
	./$(EXECUTABLE) $(TEST_C) $(TEST_LL)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LLVM_LDFLAGS) $(LLVM_INCLUDE) -o $@

$(LLVM_OBJECTS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_LDFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

part1/lex.yy.c: part1/part1.l
	lex part1/part1.l
	mv lex.yy.c part1/lex.yy.c

part1/y.tab.c part1/y.tab.h: part1/part1.y
	yacc -d part1/part1.y
	mv y.tab.h part1/y.tab.h
	mv y.tab.c part1/y.tab.c

$(TEST).ll: $(TEST).c
	clang-15 -S -emit-llvm $(TEST).c -o $(TEST).ll

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) part1/lex.yy.c part1/y.tab.c part1/y.tab.h out.ll out_new.ll out_new.s

.PHONY: all run clean
