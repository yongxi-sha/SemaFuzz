
#include <stdio.h>
#include <stdlib.h>
#include "Queue.h"

extern void FATest ();

int main (int argc, char *argv[])
{
    if (argc > 1)
    {
        printf (">>>set %s as %s \r\n", SHM_QUEUE_KEY, argv[1]);
        setenv(SHM_QUEUE_KEY, argv[1], 1);
    }
    
    InitQueue (MEMMOD_SHARE);

    ShowQueue (32);
    
    FATest ();
    return 0;
}

