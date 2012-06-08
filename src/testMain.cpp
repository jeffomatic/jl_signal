#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern void ObjectPoolUnitTest();

int main(int argc, char** argv)
{
    srand( (unsigned)time(NULL) );
    
    ObjectPoolUnitTest();
}
