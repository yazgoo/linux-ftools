#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h> 
#include <string.h> 

/*
 */
int main(int argc, char *argv[]) {

    //number of bytes we should attempt to allocate.
    size_t len = 1 * 1024 * 1024 * 1024;

    if ( argc == 2 ) 
        len = atol( argv[1] );

    printf( "Allocating %ld bytes ...", len );

    void *buff = malloc( len );

    if ( buff == NULL ) {
       printf( "\nFailed to malloc.\n" );
       exit( EXIT_FAILURE );
    }

    memset( buff, 1, len );

    printf( "done\n" );

    fflush( stdout );

    printf( "Sleeping to hold memory...\n" );
    sleep( 2147483648 );

}
