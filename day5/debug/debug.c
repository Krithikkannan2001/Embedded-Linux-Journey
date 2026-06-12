#include <stdio.h>

void greet(char *name)
{
printf("Hello, %s!\n",name);
}

int main()
{
char *person = "Krithik";
greet(person);
return 0;
}
