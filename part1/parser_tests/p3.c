extern void print(int);
extern int read();

int func(int i){
	int a;
	int b;
	a = 5;
	b = 5;	

	while (b < i){
		int a;
		a = 10 + b;
		b = b * i;
	}

	while (b < i){
		int a;
		b = b * 10;
	}
	
	return (a*b);
}
