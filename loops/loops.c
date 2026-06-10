#include<stdio.h>

int main()
{

int i;

printf("Counting up:\n");
for(i =1;i<= 5 ; i++)
{
printf("Count: %d\n",i);
}


printf("Counting down:\n");
for(i =5;i>= 1 ; i--)
{
printf("Count: %d\n",i);
}


printf("Counting Even numbers:\n");
for(i =1;i<= 10 ; i++)
{
if(i%2 == 0)
{
printf("Count: %d\n",i);
}
}

return 0;
}
