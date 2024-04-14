#include<stdio.h>

extern int func(int);

void print(int n){
	printf("%d\n", n);
}

int read(){
	int n;
	scanf("%d",&n);
	return(n); 
}

int main(){
	int n = func(20);
	printf("Returned value: %d\n", n);
	return 0;
}
