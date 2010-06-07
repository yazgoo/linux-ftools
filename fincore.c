#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux-ftools.h>

struct fincore_result 
{
    long cached_size;
};

void fincore(char* path, int pages, int summarize, int only_cached, struct fincore_result *result ) {

    int fd;
    struct stat file_stat;
    void *file_mmap;
    unsigned char *mincore_vec;
    size_t page_size = getpagesize();
    size_t page_index;

    int flags = O_RDWR;
    
    //TODO:
    //
    // pretty print integers with commas... 
    
    fd = open(path,flags);

    if ( fd == -1 ) {
        perror( NULL );
        return;
    }

    if ( fstat( fd, &file_stat ) < 0 ) {
        perror( "Could not stat file" );
        return;
    }

    file_mmap = mmap((void *)0, file_stat.st_size, PROT_NONE, MAP_SHARED, fd, 0);
    
    /*
    //FIXME: the documentation is wrong regarding this return code.
    if ( file_mmap == MAP_FAILED ) {
        perror( "Could not mmap file" );
        return 1;        
    }
    */

    mincore_vec = calloc(1, (file_stat.st_size+page_size-1)/page_size);

    if ( mincore_vec == NULL ) {
        perror( "Could not calloc" );
        return;
    }

    if ( mincore(file_mmap, file_stat.st_size, mincore_vec) != 0 ) {
        //FIXME: the documentation is wrong regarding this return code.
        //perror( "Could not call mincore on file" );
        //exit( 1 );
    }

    //printf("fincore for file %s: ",path);

    int cached = 0;
    int printed = 0;
    for (page_index = 0; page_index <= file_stat.st_size/page_size; page_index++) {
        if (mincore_vec[page_index]&1) {
            ++cached;
            if ( pages ) {
                printf("%lu ", (unsigned long)page_index);
                ++printed;
            }
        }
    }

    if ( printed ) printf("\n");

    // TODO: make all these variables long and print them as ld

    int total_pages = (file_stat.st_size/page_size);

    double cached_perc = 100 * (cached / (double)total_pages); 

    long cached_size = (double)cached * (double)page_size;

    if ( only_cached == 0 || cached > 0 ) {
        printf( "stats for %s: file size=%ld , total pages=%d , cached pages=%d , cached size=%ld, cached perc=%f \n", 
                path, file_stat.st_size, total_pages, cached, cached_size, cached_perc );
    }

    free(mincore_vec);
    munmap(file_mmap, file_stat.st_size);
    close(fd);

    result->cached_size = cached_size;

    return;

}

// print help / usage
void help() {

    fprintf( stderr, "%s version %s\n", "fincore", LINUX_FTOOLS_VERSION );
    fprintf( stderr, "fincore [options] files...\n" );
    fprintf( stderr, "\n" );

    fprintf( stderr, "  --pages=false      Don't print pages\n" );
    fprintf( stderr, "  --summarize        When comparing multiple files, print a summary report\n" );
    fprintf( stderr, "  --only-cached      Only print stats for files that are actually in cache.\n" );

}

/**

fincore [options] files...

  --pages=false      Do not print pages
  --summarize        When comparing multiple files, print a summary report
  --only-cached      Only print stats for files that are actually in cache.

root@archive102:/var/lib/mysql/blogindex# fincore --pages=false --summarize --only-cached * 
stats for CLUSTER_LOG_2010_05_21.MYI: file size=93840384 , total pages=22910 , cached pages=1 , cached size=4096, cached perc=0.004365 
stats for CLUSTER_LOG_2010_05_22.MYI: file size=417792 , total pages=102 , cached pages=1 , cached size=4096, cached perc=0.980392 
stats for CLUSTER_LOG_2010_05_23.MYI: file size=826368 , total pages=201 , cached pages=1 , cached size=4096, cached perc=0.497512 
stats for CLUSTER_LOG_2010_05_24.MYI: file size=192512 , total pages=47 , cached pages=1 , cached size=4096, cached perc=2.127660 
stats for CLUSTER_LOG_2010_06_03.MYI: file size=345088 , total pages=84 , cached pages=43 , cached size=176128, cached perc=51.190476 
stats for CLUSTER_LOG_2010_06_04.MYD: file size=1478552 , total pages=360 , cached pages=97 , cached size=397312, cached perc=26.944444 
stats for CLUSTER_LOG_2010_06_04.MYI: file size=205824 , total pages=50 , cached pages=29 , cached size=118784, cached perc=58.000000 
stats for COMMENT_CONTENT_2010_06_03.MYI: file size=100051968 , total pages=24426 , cached pages=10253 , cached size=41996288, cached perc=41.975764 
stats for COMMENT_CONTENT_2010_06_04.MYD: file size=716369644 , total pages=174894 , cached pages=79821 , cached size=326946816, cached perc=45.639645 
stats for COMMENT_CONTENT_2010_06_04.MYI: file size=56832000 , total pages=13875 , cached pages=5365 , cached size=21975040, cached perc=38.666667 
stats for FEED_CONTENT_2010_06_03.MYI: file size=1001518080 , total pages=244511 , cached pages=98975 , cached size=405401600, cached perc=40.478751 
stats for FEED_CONTENT_2010_06_04.MYD: file size=9206385684 , total pages=2247652 , cached pages=1018661 , cached size=4172435456, cached perc=45.321117 
stats for FEED_CONTENT_2010_06_04.MYI: file size=638005248 , total pages=155763 , cached pages=52912 , cached size=216727552, cached perc=33.969556 
stats for FEED_CONTENT_2010_06_04.frm: file size=9840 , total pages=2 , cached pages=3 , cached size=12288, cached perc=150.000000 
stats for PERMALINK_CONTENT_2010_06_03.MYI: file size=1035290624 , total pages=252756 , cached pages=108563 , cached size=444674048, cached perc=42.951700 
stats for PERMALINK_CONTENT_2010_06_04.MYD: file size=55619712720 , total pages=13579031 , cached pages=6590322 , cached size=26993958912, cached perc=48.533080 
stats for PERMALINK_CONTENT_2010_06_04.MYI: file size=659397632 , total pages=160985 , cached pages=54304 , cached size=222429184, cached perc=33.732335 
stats for PERMALINK_CONTENT_2010_06_04.frm: file size=10156 , total pages=2 , cached pages=3 , cached size=12288, cached perc=150.000000 
---
total cached size: 32847278080

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

    int pages         = 1;
    int summarize     = 0;
    int only_cached   = 0;

    for( ; i < argc; ++i ) {

        if ( strcmp( "--pages=false" , argv[i] ) == 0 ) {
            pages = 0;
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

    for( ; fidx < argc; ++fidx ) {

        char* path = argv[fidx];

        struct fincore_result result;

        fincore( path, pages, summarize, only_cached , &result );

        total_cached_size += result.cached_size;

    }
    
    if ( summarize ) {
        
        printf( "---\n" );

        printf( "total cached size: %ld\n", total_cached_size );

    }

}
