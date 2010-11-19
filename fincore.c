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

//char STR_FORMAT[] =  "%-80s %15s %15s %15s %15s %15s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s\n";
//char DATA_FORMAT[] = "%-80s %15ld %15d %15d %15d %15f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f\n";

char STR_FORMAT[] =  "%-80s %15s %15s %15s %15s %15s\n";
char DATA_FORMAT[] = "%-80s %15ld %15d %15d %15d %15f\n";

// program options 
int arg_pages         = 0; // display/print pages we've found.  Used for external programs.
int arg_summarize     = 1; // print a summary at the end.
int arg_only_cached   = 0; // only show cached files
int arg_graph         = 0; // graph the page distribution of files.

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

void graph(double regions[], int nr_regions ) {

    printf( "\n" );

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

    printf( "\n" );

}

void fincore(char* path, 
             struct fincore_result *result ) {

    int fd;
    struct stat file_stat;
    void *file_mmap;
    unsigned char *mincore_vec;
    size_t page_size = getpagesize();
    size_t page_index;
    int i; 

    // Array of longs of the NR of blocks per region (used for graphing)
    long *regions;

    // Array of doubles of the percentages of blocks per region.
    double *region_percs;

    // number of regions we should use.
    int nr_regions = 160;

    *regions = (int*)calloc( nr_regions , sizeof(regions) ) ;

    if ( regions == NULL ) {
        perror( "Could not allocate memory" );
        goto cleanup;      
    }

    *region_percs = calloc( nr_regions , sizeof(region_percs) ) ;

    if ( region_percs == NULL ) {
        perror( "Could not allocate memory" );
        goto cleanup;      
    }

    //TODO:
    //
    // pretty print integers with commas... 
    fd = open(path,O_RDWR);

    if ( fd == -1 ) {
        
        //FIXME: migrate this code to a decated function for dealing with errors.
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

    //init this array ...

    int region_ptr = (int)((total_pages - 1) / nr_regions);

    for (page_index = 0; page_index <= file_stat.st_size/page_size; page_index++) {

        if (mincore_vec[page_index]&1) {
            ++cached;

            if ( arg_pages ) {
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

    if ( arg_only_cached == 0 || cached > 0 ) {

        if ( arg_graph ) {
            show_headers();
        }

        printf( DATA_FORMAT, 
                path, 
                file_stat.st_size , 
                total_pages , 
                cached ,  
                cached_size , 
                cached_perc );

        for( i = 0 ; i < nr_regions; ++i ) {
            region_percs[i] = perc(regions[i], region_ptr );
        }

        if ( arg_graph ) {
            graph( region_percs, nr_regions );
        }

    }

    // update structure members when done.
    result->cached_size = cached_size;

 cleanup:

    if ( mincore_vec != NULL )
        free(mincore_vec);

    if ( file_mmap != MAP_FAILED )
        munmap(file_mmap, file_stat.st_size);

    if ( fd != -1 )
        close(fd);

    //FIXME: this is giving us an error trying to free this sometimes.
    if ( regions != NULL ) 
        free( regions );

    if ( region_percs != NULL ) 
        free( region_percs );

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

void show_headers() {

    printf( STR_FORMAT, "filename", "size", "total_pages", "cached_pages", "cached_size", "cached_perc" );
    printf( STR_FORMAT, "--------", "----", "-----------", "------------", "-----------", "-----------" );

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

    int i = 1; 

    for( ; i < argc; ++i ) {

        if ( strcmp( "--pages=false" , argv[i] ) == 0 ) {
            arg_pages = 0;
            ++fidx;
        }

        if ( strcmp( "--pages=true" , argv[i] ) == 0 ) {
            arg_pages = 1;
            ++fidx;
        }

        if ( strcmp( "--summarize" , argv[i] ) == 0 ) {
            arg_summarize = 1;
            ++fidx;
        }

        if ( strcmp( "--only-cached" , argv[i] ) == 0 ) {
            arg_only_cached = 1;
            ++fidx;
        }

        if ( strcmp( "--graph" , argv[i] ) == 0 ) {
            arg_graph = 1;
            ++fidx;
        }

        if ( strcmp( "--help" , argv[i] ) == 0 ) {
            help();
            exit(1);
        }

        //TODO what if this starts -- but we don't know what option it is?

    }

    long total_cached_size = 0;

    show_headers();

    for( ; fidx < argc; ++fidx ) {

        char* path = argv[fidx];

        struct fincore_result result;

        fincore( path, &result );

        total_cached_size += result.cached_size;

    }
    
    if ( arg_summarize ) {
        
        printf( "---\n" );

        //TODO: add more metrics including total size... 
        printf( "total cached size: %ld\n", total_cached_size );

    }

}
