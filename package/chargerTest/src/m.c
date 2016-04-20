#include <stdio.h>

void print(char dat)
{
	switch(dat)
	{
		case 1:
		{
			unsigned int a;
			unsigned  int b;
			a = 1; b = 1;
			printf("aaa\n");

			break;
		}
		case 2:
//			printf("%d\n", a+b);
			break;
		case 3:
		//	a = 3; b=3;
		//	printf("%d\n", a+b);
		break;	
	}

}

int main(void)
{

	print(1);
	print(2);
//	print(3);

}
