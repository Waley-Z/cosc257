extern void print(int);
extern int read();

int func(int i, int j){ // too many parameters
	int a;
	int b = 0; // Assignment in declaration not allowed
	int a; //multiple declarations in the same scope

	int k, l; // Multiple variables cannot be declared
	
	
	while (a < i){
		int c;
		c = 10 + b;
		int d; // All declarations should be at the beginning of a block statement.
		d = a * 2;
	}
}
