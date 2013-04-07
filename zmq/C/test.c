#include <stdio.h>
#include<string.h>
struct a
{
    char *c;
    char b;
};
int main(){
	struct a obj;
	obj.c="";
	unsigned char* s = (unsigned char*)&obj;
	printf("%d\n", strlen(s));
}
