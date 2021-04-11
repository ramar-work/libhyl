#include "megadeth.h"


//HTTP error
int http_error( struct HTTPBody *res, int status, char *fmt, ... ) { 
	va_list ap;
	char err[ 2048 ];
	memset( err, 0, sizeof( err ) );
	va_start( ap, fmt );
	vsnprintf( err, sizeof( err ), fmt, ap );
	va_end( ap );
	http_set_status( res, status ); 
	http_set_ctype( res, "text/html" );
	http_copy_content( res, err, strlen( err ) );
	http_finalize_response( res, err, strlen( err ) );
	return 0;
}



//TODO: None of these should take an error buffer.  They are just utilities...
unsigned char *read_file ( const char *filename, int *len, char *err, int errlen ) {
	//Check for and load whatever file
	int fd, fstat, bytesRead, fileSize;
	unsigned char *buf = NULL;
	struct stat sb;
	memset( &sb, 0, sizeof( struct stat ) );

	//Check for the file 
	if ( (fstat = stat( filename, &sb )) == -1 ) {
		snprintf( err, errlen, "FILE STAT ERROR: %s\n", strerror( errno ) );
		return NULL;	
	}

	//Check for the file 
	if ( (fd = open( filename, O_RDONLY )) == -1 ) {
		snprintf( err, errlen, "FILE OPEN ERROR: %s\n", strerror( errno ) );
		return NULL;	
	}

	//Allocate a buffer
	fileSize = sb.st_size + 1;
	if ( !(buf = malloc( fileSize )) || !memset(buf, 0, fileSize) ) {
		snprintf( err, errlen, "COULD NOT OPEN VIEW FILE: %s\n", strerror( errno ) );
		close( fd );
		return NULL;	
	}

	//Read the entire file into memory, b/c we'll probably have space 
	if ( (bytesRead = read( fd, buf, sb.st_size )) == -1 ) {
		snprintf( err, errlen, "COULD NOT READ ALL OF VIEW FILE: %s\n", strerror( errno ) );
		free( buf );
		close( fd );
		return NULL;	
	}

	//This should have happened before...
	if ( close( fd ) == -1 ) {
		snprintf( err, errlen, "COULD NOT CLOSE FILE %s: %s\n", filename, strerror( errno ) );
		free( buf );
		return NULL;	
	}

	*len += sb.st_size;
	return buf;
}



//Utility to add to a series of items
void * add_item_to_list( void ***list, void *element, int size, int * len ) {
	//Reallocate
	if (( (*list) = realloc( (*list), size * ( (*len) + 2 ) )) == NULL ) {
		return NULL;
	}

	(*list)[ *len ] = element; 
	(*list)[ (*len) + 1 ] = NULL; 
	(*len) += 1; 
	return list;
}


#if 0
const int serve_static( int dd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn ) {
	(void)dd;
	struct stat sb;
	int fd = 0;
	int size = 0;
	char err[ 2048 ] = { 0 };
	char fpath[ 2048 ] = { 0 };
	char *fname = rq->path;
	char *extension = NULL;
	const char *mimetype_default = mmtref( "application/octet-stream" );
	const char *mimetype = NULL;
	uint8_t *content = NULL;
	struct lconfig *host = conn->hconfig;

	//Die if no host directory is specified.
	if ( !host->dir ) {
		return http_set_error( rs, 500, "No directory path specified for this site." );
	}

	//Stop / requests when dealing with static servers
	//FPRINTF( "rq->path is %s\n", rq->path );
	if ( !rq->path || ( strlen( rq->path ) == 1 && *rq->path == '/' ) ) {
		//Check for a default page (like index.html, which comes from config)
		if ( !host->root_default ) {
			return http_set_error( rs, 404, "No default root specified for this site." );
		}
		fname = (char *)host->root_default; 
	}
		
	//Create a fullpath
	if ( snprintf( fpath, sizeof(fpath) - 1, "%s%s", host->dir, fname ) == -1 ) {
		return http_set_error( rs, 500, "Full filepath probably truncated." );
	}

	//Crudely check the extension before serving.
	//FPRINTF( "Made request for file at path: %s\n", fpath );
	mimetype = mimetype_default;
	if ( ( extension = getExtension( fpath ) ) ) {
		extension++;
		if ( ( mimetype = mmimetype_from_file( extension ) ) == NULL ) {
			mimetype = mimetype_default;
		}
	}
	
	//Check for the file 
	if ( stat( fpath, &sb ) == -1 ) {
		snprintf( err, sizeof( err ), "%s: %s.", strerror( errno ), fpath );
		return http_set_error( rs, 404, err );
	}

	//If the file is zero-length, just kill it there
	if ( sb.st_size == 0 ) {
		snprintf( err, sizeof( err ), "File at path: %s is zero-length.", fpath );
		return http_set_error( rs, 200, err );
	}

	//Check for the file 
	if ( ( fd = open( fpath, O_RDONLY ) ) == -1 ) {
		snprintf( err, sizeof( err ), "%s: %s.", strerror( errno ), fpath );
		//depends on type of problem (permission, corrupt, etc)
		return http_set_error( rs, 500, err );
	}

	//Allocate a buffer
	if ( !( content = malloc( sb.st_size ) ) || !memset( content, 0, sb.st_size ) ) {
		const char fmt[] = "Could not allocate space for file: %s\n";
		snprintf( err, sizeof( err ), fmt, strerror( errno ) );
		return http_set_error( rs, 500, err );
	}

	//Read the entire file into memory, b/c we'll probably have space 
	if ( ( size = read( fd, content, sb.st_size )) == -1 ) {
		const char fmt[] = "Could not read all of file %s: %s.";
		snprintf( err, sizeof( err ), fmt, fpath, strerror( errno ) );
		free( content );
		return http_set_error( rs, 500, err );
	}

	//This should have happened before...
	if ( close( fd ) == -1 ) {
		const char fmt[] = "Could not close file %s: %s\n";
		snprintf( err, sizeof( err ), fmt, fpath, strerror( errno ) );
		free( content );
		return http_set_error( rs, 500, err );
	}

	//Just set content messages...
	http_set_status( rs, 200 );
	http_set_ctype( rs, mimetype );
	http_set_content( rs, content, size ); 

	if ( !http_finalize_response( rs, err, sizeof( err ) ) ) {
		free( content );
		return http_set_error( rs, 500, err );
	}

	return 1;
}
#endif
