extern void print(int);
extern int read();

int func(int n){
	int a;
	int b;
	int i;
	a = 1;
	b = 1;
	i = 0;

	while (i < n) {
		int t;
		print(a);
		i = i + 1;

		t = a + b;
		a = b;
		b = t;
	}
	
	return i;
}
