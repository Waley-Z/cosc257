extern void print(int);
extern int read();

int func(int i){
	int a;
	int b;
	
	b = 0;
	while (b < i){
		int c;
		c = read();
		b = 10 + c;
	}
	return(c);
}
