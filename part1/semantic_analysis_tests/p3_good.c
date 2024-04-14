extern void print(int);
extern int read();

int func(int i){
	int a;
	int b;
	a = 5;
	b = 2;

	if (a < i){
		int a;
		while (b < i){
			int a;
			a = b + 20;
			b = a;
		}
		a = 10 + b;
	}
	else {
		if (b < i) 
			b = a;
	}
	
	return 1;
}
