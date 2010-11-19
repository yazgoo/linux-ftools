#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux-ftools.h>
#include <math.h>
#include <errno.h>

char STR_FORMAT[] =  "%-80s %15s %15s %15s %15s %15s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s\n";
char DATA_FORMAT[] = "%-80s %15ld %15d %15d %15d %15f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f\n";

struct fincore_result 
{
    long cached_size;
};

char *_itoa(int n) {
	static char retbuf[100];
	sprintf(retbuf, "%d", n);
	return retbuf;
}

char *_ltoa( long value ) {

    static char buff[100];

    sprintf( buff, "%ld", value );

    return buff;

}

char *_dtoa( double value ) {

    static char buff[100];

    sprintf( buff, "%f", value );

    return buff;

}

double perc( long val, int range ) {
    
    if ( range == 0 )
        return 0;

    double result = ( val / (double)range ) * 100;

    return result;

}

void art(double regions[], int nr_regions ) {

    int *ptr;

    int i;
    for( i = 0 ; i < 10; ++ i ) {

        double perc_index = 100 - ((i+1) * 10 );

        // show where we are.
        printf( "%4.0f %% ", perc_index + 10 );

        int j;

        for( j = 0; j < nr_regions; ++j ) {

            if ( regions[j] >= perc_index ) {
                printf( "*" );
            } else {
                printf( " " );
            }

        }
        
        printf( "\n" );

    }
    
}

void fincore(char* path, 
             int pages, 
             int summarize, 
             int only_cached, 
             struct fincore_result *result ) {

    int fd;
    struct stat file_stat;
    void *file_mmap;
    unsigned char *mincore_vec;
    size_t page_size = getpagesize();
    size_t page_index;
    int i; 

    int flags = O_RDWR;
    
    //TODO:
    //
    // pretty print integers with commas... 
    fd = open(path,flags);

    if ( fd == -1 ) {

        char buff[1024];
        sprintf( buff, "Could not open file: %s", path );
        perror( buff );

        return;
    }

    if ( fstat( fd, &file_stat ) < 0 ) {

        char buff[1024];
        sprintf( buff, "Could not stat file: %s", path );
        perror( buff );

        goto cleanup;

    }

    file_mmap = mmap((void *)0, file_stat.st_size, PROT_NONE, MAP_SHARED, fd, 0 );
    
    if ( file_mmap == MAP_FAILED ) {

        char buff[1024];
        sprintf( buff, "Could not mmap file: %s", path );
        perror( buff );
        goto cleanup;      
    }

    mincore_vec = calloc(1, (file_stat.st_size+page_size-1)/page_size);

    if ( mincore_vec == NULL ) {
        //something is really wrong here.  Just exit.
        perror( "Could not calloc" );
        exit( 1 );
    }

    if ( mincore(file_mmap, file_stat.st_size, mincore_vec) != 0 ) {
        char buff[1024];
        sprintf( buff, "%s: Could not call mincore on file", path );
        perror( buff );
        exit( 1 );
    }

    int total_pages = (int)ceil( (double)file_stat.st_size / (double)page_size );

    int cached = 0;
    int printed = 0;

    int nr_regions = 10;

    long regions[nr_regions] ;

    /*
    for( i = 0; i < nr_regions; ++i ) {
        regions[i] = 0;
    }
    */

    //init this array ...

    fflush( stdout );

    int region_ptr = (int)((total_pages - 1) / nr_regions);

    for (page_index = 0; page_index <= file_stat.st_size/page_size; page_index++) {

        if (mincore_vec[page_index]&1) {
            ++cached;

            if ( pages ) {
                printf("%lu ", (unsigned long)page_index);
                ++printed;
            }

            if ( region_ptr > 0 ) {

                int region = (int)(page_index / region_ptr );

                ++regions[region];

            }
        }

    }

    if ( printed ) printf("\n");

    // TODO: make all these variables long and print them as ld

    double cached_perc = 100 * (cached / (double)total_pages); 

    long cached_size = (long)cached * (long)page_size;

    if ( only_cached == 0 || cached > 0 ) {

        /*
        printf( "FIXME: r9:          %d \n" , regions[9] );
        printf( "FIXME: r9 perc:     %f \n" , perc( regions[9], region_ptr ) );
        */

        /*
        printf( "FIXME: cached:      %d \n" , cached );
        printf( "FIXME: total_pages: %d \n" , total_pages );
        printf( "FIXME: region_ptr:  %d \n" , region_ptr );

        int i;
        for ( i = 0; i < 10; ++i ) {
            printf( "FIXME: region %d = %ld perc=%f\n" , i, regions[i], perc( regions[i], region_ptr ) );
        }
        */

        printf( DATA_FORMAT, 
                path, 
                file_stat.st_size , 
                total_pages , 
                cached ,  
                cached_size , 
                cached_perc ,
                perc(regions[0], region_ptr ),
                perc(regions[1], region_ptr ) , 
                perc(regions[2], region_ptr ) , 
                perc(regions[3], region_ptr ) , 
                perc(regions[4], region_ptr ) , 
                perc(regions[5], region_ptr ) , 
                perc(regions[6], region_ptr ) ,
                perc(regions[7], region_ptr ) ,
                perc(regions[8], region_ptr ) ,
                perc(regions[9], region_ptr ) );

        double region_percs[10];
        
        for( i = 0 ; i < 10; ++i ) {
            region_percs[i] = perc(regions[i], region_ptr );
        }

        art( region_percs, nr_regions );

    }

    result->cached_size = cached_size;

 cleanup:

    if ( mincore_vec != NULL )
        free(mincore_vec);

    if ( file_mmap != MAP_FAILED )
        munmap(file_mmap, file_stat.st_size);

    if ( fd != -1 )
        close(fd);

    return;

}

// print help / usage
void help() {

    fprintf( stderr, "%s version %s\n", "fincore", LINUX_FTOOLS_VERSION );
    fprintf( stderr, "fincore [options] files...\n" );
    fprintf( stderr, "\n" );

    fprintf( stderr, "  --pages=true|false    Don't print pages\n" );
    fprintf( stderr, "  --summarize           When comparing multiple files, print a summary report\n" );
    fprintf( stderr, "  --only-cached         Only print stats for files that are actually in cache.\n" );

}

/**
 * see README
 */
int main(int argc, char *argv[]) {

    if ( argc == 1 ) {
        help();
        exit(1);
    }

    //keep track of the file index.
    int fidx = 1;

    // parse command line options.

    //TODO options:
    //
    // --pages=true|false print/don't print pages
    // --summarize when comparing multiple files, print a summary report
    // --only-cached only print stats for files that are actually in cache.

    int i = 1; 

    int pages         = 0;
    int summarize     = 1;
    int only_cached   = 0;

    for( ; i < argc; ++i ) {

        if ( strcmp( "--pages=false" , argv[i] ) == 0 ) {
            pages = 0;
            ++fidx;
        }

        if ( strcmp( "--pages=true" , argv[i] ) == 0 ) {
            pages = 1;
            ++fidx;
        }

        if ( strcmp( "--summarize" , argv[i] ) == 0 ) {
            summarize = 1;
            ++fidx;
        }

        if ( strcmp( "--only-cached" , argv[i] ) == 0 ) {
            only_cached = 1;
            ++fidx;
        }

        if ( strcmp( "--help" , argv[i] ) == 0 ) {
            help();
            exit(1);
        }

        //TODO what if this starts -- but we don't know what option it is?

    }

    long total_cached_size = 0;

    printf( STR_FORMAT, "filename", "size", "total_pages", "cached_pages", "cached_size", "cached_perc", "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9" );
    printf( STR_FORMAT, "--------", "----", "-----------", "------------", "-----------", "-----------", "--", "--" , "--", "--" , "--", "--" , "--", "--" , "--", "--" );

    for( ; fidx < argc; ++fidx ) {

        char* path = argv[fidx];

        struct fincore_result result;

        fincore( path, pages, summarize, only_cached , &result );

        total_cached_size += result.cached_size;

    }
    
    if ( summarize ) {
        
        printf( "---\n" );

        //TODO: add more metrics including total size... 
        printf( "total cached size: %ld\n", total_cached_size );

    }

}
