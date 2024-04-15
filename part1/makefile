source = part1
$(source).out: $(source).l $(source).y
	yacc -d $(source).y
	lex $(source).l
	g++ -o $(source).out lex.yy.c y.tab.c ast.cpp part1_semantic.cpp -g

clean:
	rm lex.yy.c y.tab.c y.tab.h $(source).out
