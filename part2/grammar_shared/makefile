filename = minic
(filename).out: $(filename).l $(filename).y ast.c ast.h
	yacc -d -t $(filename).y
	lex $(filename).l
	g++ -g lex.yy.c y.tab.c ast.c -o $(filename).out

clean:
	rm -rf $(filename).out lex.yy.c y.tab.c y.tab.h
