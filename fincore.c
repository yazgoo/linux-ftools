
#include <ctype.h>
#include <getopt.h>
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

char STR_FORMAT[] =  "%-80s %18s %18s %18s %18s %18s %18s\n";
char DATA_FORMAT[] = "%-80s %'18ld %'18d %'18ld %'18ld %'18ld %18.2f\n";

long DEFAULT_NR_REGIONS       = 160;  // default number of regions

// program options 
int arg_pages                 = 0;    // display/print pages we've found.  Used for external programs.
int arg_summarize             = 1;    // print a summary at the end.
int arg_only_cached           = 0;    // only show cached files
int arg_graph                 = 0;    // graph the page distribution of files.
int arg_verbose               = 0;    // level of verbosity.
int arg_vertical              = 0;    // print variables vertical

long arg_min_size             = -1;   // required minimum size for files or we ignore them.
long arg_min_cached_size      = -1;   // required minimum percent cached for files or we ignore them.
long arg_min_cached_perc      = -1;   // required minimum percent of a file in cache.

long nr_regions               = 0;

// Array of longs of the NR of blocks per region (used for graphing)
static long *regions          = NULL;

// Array of doubles of the percentages of blocks per region.
static double *region_percs   = NULL;

//TODO:
//
// - ability to adjust print graph width and height width from the command line.
//
// - convert ALL the variables used to store stats to longs.
//
// - option to print symbols in human readable form (1GB, 2.1GB , 300KB
//
// - only call setlocale() if it's not currently set.
// 
// - print new headers every N rows.

struct fincore_result 
{
    long cached_size;
};

int _argtobool( char* str ) {

    int result = 0;

    if ( optarg != NULL ) {

        if ( strcmp( "true", optarg ) == 0 ) {
            result = 1;
        } 

        if ( strcmp( "1", optarg ) == 0 ) {
            result = 1;
        } 

    } else {
        result = 1;
    }

    return result;
}

int _argtoint( char* str, int _default ) {

    int result = _default;

    if ( optarg != NULL ) {
        result = atoi( str );
    } 

    return result;
}

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
    size_t cached = 0;

    // the number of pages that we have printed 
    size_t printed = 0;

    //the total number of pages we're working with
    size_t total_pages;

    //the oldest page in the cache or -1 if it isn't in the cache.
    off_t min_cached_page = -1 ;

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

    size_t calloc_size = (file_stat.st_size+page_size-1)/page_size;

    mincore_vec = calloc(1, calloc_size);

    if ( mincore_vec == NULL ) {
        perror( "Could not calloc" );
        exit( 1 );
    }

    if ( mincore(file_mmap, file_stat.st_size, mincore_vec) != 0 ) {
        char buff[1024];
        sprintf( buff, "%s: Could not call mincore on file", path );
        perror( buff );
        exit( 1 );
    }

    total_pages = (int)ceil( (double)file_stat.st_size / (double)page_size );

    int region_ptr = (int)((total_pages - 1) / nr_regions);

    for (page_index = 0; page_index <= file_stat.st_size/page_size; page_index++) {

        if (mincore_vec[page_index]&1) {

            ++cached;

            if ( min_cached_page == -1 || page_index < min_cached_page ) {
                min_cached_page = page_index;
            }

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

    size_t cached_size = (long)cached * (long)page_size;

    if ( arg_min_cached_size > 0 && cached_size <= arg_min_cached_size ) {
        goto cleanup;
    }

    if ( arg_min_cached_perc > 0 && cached_perc <= arg_min_cached_perc ) {
        goto cleanup;
    }

    if ( arg_min_size > 0 && file_stat.st_size <= arg_min_size ) {
        goto cleanup;
    }

    if ( arg_only_cached == 0 || cached > 0 ) {

        if ( arg_graph ) {
            _show_headers();
        }

        if ( arg_vertical ) {

            printf( "%s\n", path );
            printf( "size: %'ld\n", file_stat.st_size );
            printf( "total_pages: %'ld\n", total_pages );
            printf( "min_cached_page: %'ld\n", min_cached_page );
            printf( "cached: %'ld\n", cached );
            printf( "cached_size: %'ld\n", cached_size );
            printf( "cached_perc: %.2f\n", cached_perc );

        } else { 

            printf( DATA_FORMAT, 
                    path, 
                    file_stat.st_size , 
                    total_pages , 
                    min_cached_page,
                    cached ,  
                    cached_size , 
                    cached_perc );

        }

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

    if ( mincore_vec != NULL ) {
        free(mincore_vec);
        mincore_vec = NULL;
    }

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

    fprintf( stderr, "  -s --summarize          When comparing multiple files, print a summary report\n" );
    fprintf( stderr, "  -p --pages              Print pages that are cached\n" );
    fprintf( stderr, "  -o --only-cached        Only print stats for files that are actually in cache.\n" );
    fprintf( stderr, "  -g --graph              Print a visual graph of each file's cached page distribution.\n" );
    fprintf( stderr, "  -S --min-size           Require that each files size be larger than N bytes.\n" );
    fprintf( stderr, "  -C --min-cached-size    Require that each files cached size be larger than N bytes.\n" );
    fprintf( stderr, "  -P --min-perc-cached    Require percentage of a file that must be cached.\n" );
    fprintf( stderr, "  -h --help               Print this message.\n" );
    fprintf( stderr, "  -L --vertical           Print the output of this script vertically.\n" );
    fprintf( stderr, "\n" );

}

void _show_headers() {

    printf( STR_FORMAT, "filename", "size", "total_pages", "min_cached page", "cached_pages", "cached_size", "cached_perc" );
    printf( STR_FORMAT, "--------", "----", "-----------", "---------------", "------------", "-----------", "-----------" );
    
    return;

}

/**
 * see README
 */
int main(int argc, char *argv[]) {

    int c;

    static struct option long_options[] =
        {
            /* These options don't set a flag.
               We distinguish them by their indices. */
            {"summarize",         optional_argument,       0, 's'},
            {"pages",             optional_argument,       0, 'p'},
            {"only-cached",       optional_argument,       0, 'c'},
            {"graph",             optional_argument,       0, 'g'},
            {"verbose",           optional_argument,       0, 'v'},
            {"min-size",          required_argument,       0, 'S'},
            {"min-cached-size",   required_argument,       0, 'C'},
            {"min-cached-perc",   required_argument,       0, 'P'},
            {"help",              no_argument,             0, 'h'},
            {"vertical",          no_argument,             0, 'L'},
            {0, 0, 0, 0}
        };

    while (1) {

        /* getopt_long stores the option index here. */
        int option_index = 0;
        
        c = getopt_long (argc, argv, "s::p::c::g::v::L::S:C:P:h",long_options, &option_index);
        
        /* Detect the end of the options. */
        if (c == -1)
            break;
        
        //bool foo = 1;

        switch (c)
            {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;

            case 'p':
                arg_pages = _argtobool( optarg );
                break;

            case 's':
                arg_summarize = _argtobool( optarg );
                break;
                
            case 'c':
                arg_only_cached = _argtobool( optarg );
                break;
                
            case 'g':
                arg_graph = _argtobool( optarg );
                break;

            case 'v':
                arg_verbose = _argtoint( optarg, 1 );
                break;

            case 'S':
                arg_min_size = _argtoint( optarg, 0 );
                break;

            case 'C':
                arg_min_cached_size = _argtoint( optarg, 0 );
                break;

            case 'P':
                arg_min_cached_perc = _argtoint( optarg, 0 );
                break;

            case 'L':
                arg_vertical = _argtoint( optarg, 1 );
                break;

            case 'h':
                help();
                exit(1);

            case '?':
                /* getopt_long already printed an error message. */
                break;
                
            default:

                fprintf( stderr, "Invalid command line item: %s\n" , argv[ optind ] );
                help();
                
                exit(1);
            }
    } // done processing arguments.

   if ( arg_verbose >= 1 ) {

       printf( "Running with arguments: \n" );
       printf( "    pages:            %d\n",  arg_pages );
       printf( "    summarize:        %d\n",  arg_summarize );
       printf( "    only cached:      %d\n",  arg_only_cached );
       printf( "    graph:            %d\n",  arg_graph );
       printf( "    min size:         %ld\n", arg_min_size );
       printf( "    min cached size:  %ld\n", arg_min_cached_size );
       printf( "    min cached perc:  %ld\n", arg_min_cached_perc );
       printf( "    vertical:         %ld\n", arg_vertical );

   }

    setlocale( LC_NUMERIC, "en_US" );

    nr_regions = DEFAULT_NR_REGIONS;

    struct winsize ws; 

    if (ioctl(stdout,TIOCGWINSZ,&ws) == 0) { 
        
        if ( ws.ws_col > nr_regions ) {
            nr_regions = ((ws.ws_col/2)*2) - 10;
        }

        //printf( "Using graph width: %d\n" , nr_regions );

    } 

    if ( optind == argc ) {
        help();
        exit(1);
    }

    if ( ! arg_graph ) 
        _show_headers();

    long total_cached_size = 0;
    
    /* Print any remaining command line arguments (not options). */
        
    if (optind < argc) {
        
       while (optind < argc) {
           
           char* path = argv[optind++];

            struct fincore_result result;

            fincore( path, &result );

            total_cached_size += result.cached_size;

           //printf ("%s ", );
           //putchar ('\n');
       }

   }

    if ( arg_summarize ) {
        
        printf( "---\n" );

        //TODO: add more metrics including total size... 
        printf( "total cached size: %'ld\n", total_cached_size );

    }

}
