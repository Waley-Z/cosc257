#include"ast.h"
#include<stdio.h>

int main(){
	vector<astNode*> *slist;
	slist = new vector<astNode*> ();

	astNode *a11 = createVar("test1"); //create a variable node test1
	astNode *a12 = createCnst(20); //create a integer constant
	astNode *a13 = createVar("test3"); //create a variable node test1
	
	astNode *a14 = createBExpr(a11, a12, add); //test1 + 20
	astNode *a15 = createAsgn(a13, a14);	//test3 = test1 + 20
	slist->push_back(a15); // Add to lists of statements
	
	astNode *a21 = createDecl("local");
	slist->push_back(a21);


	astNode *a31 = createVar("test31");
	astNode *a32 = createUExpr(a31, uminus);
	astNode *a33 = createRet(a32);
	slist->push_back(a33);

	astNode *a51 = createVar("test51");
	astNode *a52 = createCall("read");
	astNode *a53 = createAsgn(a51, a52);	
 	slist->push_back(a53);
	
	astNode *a8 = createBlock(slist); //Create a block statement

	astNode *a7 = createFunc("func_name1", NULL, a8);

	printNode(a7);
	
	freeNode(a7);
}
