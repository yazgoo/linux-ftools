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
#include <locale.h>
#include <sys/ioctl.h> 

char STR_FORMAT[] =  "%-80s %18s %18s %18s %18s %18s\n";
char DATA_FORMAT[] = "%-80s %'18ld %'18d %'18d %'18ld %18f\n";

// program options 
int arg_pages                 = 0;    // display/print pages we've found.  Used for external programs.
int arg_summarize             = 1;    // print a summary at the end.
int arg_only_cached           = 0;    // only show cached files
int arg_graph                 = 0;    // graph the page distribution of files.

long DEFAULT_NR_REGIONS       = 160;  // default number of regions

long arg_min_size             = -1;   // required minimum size for files or we ignore them.
long arg_min_perc_cached      = -1;   // required minimum percent cached for files or we ignore them.

long nr_regions               = 0;

// Array of longs of the NR of blocks per region (used for graphing)
static long *regions          = NULL;

// Array of doubles of the percentages of blocks per region.
static double *region_percs   = NULL;

//TODO:
//
// - pretty print integers with commas... 
// 
// - ability to print graph width and height width
//
// - convert ALL the variables used to store stats to longs.
//
// - option to print symbols in human readable form (1GB, 2.1GB , 300KB
//
// - we trip a bug with free(regions) but only when it's not 106... what is this
//   about?   This prevents custom graph widths.
//
// - only call setlocale() if it's not currently set.

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

void graph_header( long nr_regions ) {

    int j;

    printf( " ------" );
    for( j = 0; j < nr_regions; ++j ) {
        printf( "-" );
    }

    printf( "\n" );

}

void graph(double regions[], long nr_regions ) {

    printf( "\n" );

    graph_header(nr_regions);

    int *ptr;

    int i, j;
    
    int perc_width = 10;

    for( i = 0 ; i < perc_width; ++ i ) {

        double perc_index = 100 - ((i+1) * perc_width );

        // show where we are.
        printf( "%4.0f %% ", perc_index + perc_width );

        for( j = 0; j < nr_regions; ++j ) {

            double val = regions[j];

            if ( val < 1 ) {
                printf( " " );
            } else if ( val >= perc_index ) {
                printf( "*" );
            } else {
                printf( " " );
            }

        }
        
        printf( " | \n" );

    }

    graph_header(nr_regions);

    printf( "\n" );

}

void fincore(char* path, 
             struct fincore_result *result ) {

    int fd;
    struct stat file_stat;
    void *file_mmap;

    // vector result from mincore
    unsigned char *mincore_vec;

    // default page size of this OS
    size_t page_size = getpagesize();
    size_t page_index;

    int i; 

    // the number of pages that we have detected that are cached 
    // FIXME: should be a long
    unsigned int cached = 0;
    // the number of pages that we have printed 
    // FIXME: should be a long
    unsigned int printed = 0;

    // by default the cached size is zero so initialize this member.
    result->cached_size = 0;

    //FIXME: make this sizeof(regions)
    if ( regions == NULL )
        regions = calloc( nr_regions , sizeof(long) ) ;

    if ( region_percs == NULL )
        region_percs = calloc( nr_regions , sizeof(double) ) ;

    if ( regions == NULL || region_percs == NULL ) {
        perror( "Could not allocate memory" );
        goto cleanup;      
    }

    for( i = 0; i < nr_regions; ++i ) {
        regions[i] = (long)0;
        region_percs[i] = (double)0;
    }

    fd = open( path, O_RDONLY );

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

    if ( file_stat.st_size == 0 ) {
        // this file could not have been cached as it's empty so anything we
        //would do is pointless.
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
        //FIXME: print the file we're running from.
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

        if ( arg_graph && total_pages > nr_regions ) {
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
    fprintf( stderr, "  --graph               Print a visual graph of each file's cached page distribution.\n" );

}

void show_headers() {

    printf( STR_FORMAT, "filename", "size", "total_pages", "cached_pages", "cached_size", "cached_perc" );
    printf( STR_FORMAT, "--------", "----", "-----------", "------------", "-----------", "-----------" );

}

/**
 * see README
 */
int main(int argc, char *argv[]) {

    setlocale( LC_NUMERIC, "en_US" );

    nr_regions = DEFAULT_NR_REGIONS;

    struct winsize ws; 

    if (ioctl(stdout,TIOCGWINSZ,&ws) == 0) { 
        
        if ( ws.ws_col > nr_regions ) {
            nr_regions = ((ws.ws_col/2)*2) - 10;
        }

        //printf( "Using graph width: %d\n" , nr_regions );

    } 

    int i = 1; 

    if ( argc == 1 ) {
        help();
        exit(1);
    }

    //keep track of the file index.
    int fidx = 1;

    // parse command line options (TODO: move to GNU getopt)

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

        if ( strstr( "--min-size=" , argv[i] ) != NULL ) {
            //arg_min_size = atoi( ;
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

    if ( ! arg_graph ) 
        show_headers();

    long total_cached_size = 0;

    //TODO: mean cached percentage

    for( ; fidx < argc; ++fidx ) {

        char* path = argv[fidx];

        struct fincore_result result;

        fincore( path, &result );

        total_cached_size += result.cached_size;

    }
    
    if ( arg_summarize ) {
        
        printf( "---\n" );

        //TODO: add more metrics including total size... 
        printf( "total cached size: %'ld\n", total_cached_size );

    }

}
