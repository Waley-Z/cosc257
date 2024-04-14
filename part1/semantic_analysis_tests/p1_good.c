extern void print(int);
extern int read();

int func(int i){
	int a;
	int b;
	a = 5;
	b = 2;

	if (a < i){
		while (b < i){
			b = b + 20;
		}
		a = 10 + b;
	}
	else {
		if (b < i) 
			b = a;
	}
	
	return 1;
}
