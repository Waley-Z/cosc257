source = part1
$(source): $(source).l $(source).y semantic.cpp main.cpp
	yacc -d $(source).y
	lex $(source).l
	g++ -o $@ lex.yy.c y.tab.c ast.cpp semantic.cpp main.cpp -g

clean:
	rm lex.yy.c y.tab.c y.tab.h $(source)
