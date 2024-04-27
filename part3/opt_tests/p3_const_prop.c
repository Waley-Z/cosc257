extern void print(int);
extern int read();

int func(int p){
	int a;
	int b;
	int c1;
	a = 10;
	b = 20;
  c1 = a + b;

	if (a<p){
		b = 30;
	}
	else {
		b = c1;
	}
	
	return (b+a);
}
