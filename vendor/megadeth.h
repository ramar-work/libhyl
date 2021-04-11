//this is more and more like a generalized library...

#include <zhttp.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef MEGADETH_H

#define add_item(LIST,ELEMENT,SIZE,LEN) \
 add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )

int http_error( struct HTTPBody *, int, char *, ... );
unsigned char *read_file ( const char *, int *, char *, int );
void * add_item_to_list( void ***, void *, int, int *);

#endif
