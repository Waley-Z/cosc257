extern void print(int);
extern int read();

int func(int p){
	int a;
	int b;
	a = 0;

	if (p < 0)
		a = 10;
	else {
		int a;
		a = 2;
		b = 0;
		while (b < p) 
			b = b + a;
	}
	return a+b;
}
