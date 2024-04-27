extern void print(int);
extern int read();

int func(int i){
	int a;
	int b;
	int c;
	
	a = 10;
	b = 15;
	c = a + b;
	a = 5;

	while (a < i){
		a = a + 1;
		if (a > b) c = 25;
		else c = 25; 
		
	}
	print(a);
	print(b);
	print(c);
	return (b+c);
}
