#include <linux-ftools.h>

/** 

SYNTAX: fallocate file length

fallocate() allows the caller to directly manipulate the allocated disk space
for the file referred to by fd for the byte range starting at offset and
continuing for len bytes.

The mode argument determines the operation to be performed on the given
range. Currently only one flag is supported for mode:

FALLOC_FL_KEEP_SIZE

This flag allocates and initializes to zero the disk space within the range
specified by offset and len. After a successful call, subsequent writes into
this range are guaranteed not to fail because of lack of disk
space. Preallocating zeroed blocks beyond the end of the file is useful for
optimizing append workloads. Preallocating blocks does not change the file size
(as reported by stat(2)) even if it is less than offset+len.

If FALLOC_FL_KEEP_SIZE flag is not specified in mode, the default behavior is
almost same as when this flag is specified. The only difference is that on
success, the file size will be changed if offset + len is greater than the file
size. This default behavior closely resembles the behavior of the
posix_fallocate(3) library function, and is intended as a method of optimally
implementing that function.

Because allocation is done in block size chunks, fallocate() may allocate a
larger range than that which was specified.

*/


// wrapper function for strerror(errno)
char* make_error_string(char* message) {

    char* error_string;

    if( (asprintf(&error_string, "%s: %s", message, strerror(errno))) == -1)
    {   
        fprintf (stderr,"asprintf: out of memory\n");
        exit(1);
    }

    return error_string;
}


void logstats(int fd) {

    struct stat fd_stat ;

    if ( fstat( fd, &fd_stat )  < 0 ) {
        perror( "Could not stat file" );
        exit(1);
    }

    printf( "File stats: \n" );
    printf( "    Length:            %ld\n", fd_stat.st_size );
    printf( "    Block size:        %ld\n", fd_stat.st_blksize );
    printf( "    Blocks allocated:  %ld\n" , fd_stat.st_blocks );

}

int main(int argc, char *argv[]) {

    if ( argc != 3 ) {
        //fprintf( stderr, "sizeof %ll\n", sizeof(loff_t));
        fprintf( stderr, "%s version %s\n", argv[0], LINUX_FTOOLS_VERSION );
        fprintf( stderr, "SYNTAX: fallocate file length\n" );
        exit(1);
    }

    char* path = argv[1];

    printf( "Going to fallocate %s\n", path );

    int flags = O_RDWR;
    int fd = open( path, flags );

    if ( fd == -1 ) {
        perror( "Unable to open file" );
        exit(1);
    }

    logstats(fd);

    size_t length = atol( argv[2] );

    printf( "Increasing file to: %ld\n", length );

    if ( length <= 0 ) {
        fprintf( stderr, "Unable to allocate size %ld\n", length );
        exit( 1 );
    }

    fallocate_result my_result;
    my_result = fallocate(path, length);
    long result = my_result.return_value;
    
    if ( result != 0 ) {

        //TODO: rework this error handling
        if ( result != -1 ) {
            errno=result;
            perror( "Unable to fallocate" );
            exit( 1 );
        } else {
            char buff[100];
            sprintf( buff, "Unable to fallocate: %ld\n" , result );
            perror( buff );
            exit( 1 );
        }

        return 1;
    }

    logstats(fd);

    close( fd );
    return 0;

}


fallocate_result fallocate(char* path, unsigned long length) {

    fallocate_result result;
    result.error_string = NULL;
    result.return_value = 0;
    result.error_state  = FALSE;

    int flags = O_RDWR;
    int fd = open( path, flags );

    if ( fd == -1 ) {
        result.error_string = make_error_string("Unable to fallocate: Unable to open file");
        result.error_state  = TRUE;
        goto cleanup;
    }

    if ( length <= 0 ) {
        result.error_string = "Invalid allocation size";
        result.return_value = 1;
        result.error_state  = TRUE;
        goto cleanup;
    }

    loff_t offset = 0;

    int mode = FALLOC_FL_KEEP_SIZE;

    result.return_value = syscall( SYS_fallocate, fd, mode, offset, length );

    if ( result.return_value != 0 ) {
        errno=result.return_value;

        if ( result.return_value != -1 ) {
            result.error_string = make_error_string("Unable to fallocate");
            result.error_state  = TRUE;
            goto cleanup;
        } else {
            char buff[300];
            sprintf( buff, "Unable to fallocate: %ld\n" , result.return_value );
            result.error_string = make_error_string(buff);
            result.error_state  = TRUE;
            goto cleanup;
        }

        result.error_string = make_error_string("Unable to fallocate");
        result.error_state  = TRUE;
        goto cleanup;

    }

 cleanup:
    close(fd);
    return result;
}

