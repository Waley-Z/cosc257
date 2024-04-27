#include<stdio.h>

int func(int);

int read(){
	int x;
	scanf("%d", &x); 
	return x;
}

void print(int x){
	printf("%d\n", x);
}

int main(){
	int i = func(5);
	printf("In main printing return value of test: %d\n", i);
	return 0;
}
