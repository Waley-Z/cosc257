extern void print(int);
extern int read();

int func(int i){
	int a;
	int b;
	int i;

	a = 10;
    b = 5;
	
	while (a < i){
		int c;
		while (b < i){
			b = b + 20;
			print(b);
		}
		a = b + 10;
	}
	
	return a+b;
}
