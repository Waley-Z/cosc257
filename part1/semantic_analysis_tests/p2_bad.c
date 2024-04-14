extern void print(int);
extern int read();

int func(int i){
	int a;
	
	a = read();
	b = 5;
	
	while (a < i){
		a = a + b;
		print(b);
	}
	return a;
}
