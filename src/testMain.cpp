#include <stdlib.h>
#include <stdio.h>
#include <time.h>

extern void ObjectPoolUnitTest();

int main(int argc, char** argv)
{
    srand( (unsigned)time(NULL) );    
    ObjectPoolUnitTest();
    
    printf("\nDone! Press enter to continue...\n");
    getchar();    
}
