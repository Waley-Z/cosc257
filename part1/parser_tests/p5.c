extern void print(int);
extern int read();

int func(int i){
	int a;
	int b;
	
	b = 0;
	while (b < i){
		int a;
		a = read();
		b = 10 + a;
	}
	return(b);
}
