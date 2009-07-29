#include "sysdep1.h"	/* here to get stat64 on some badly designed Linux systems */
#include "f2c.h"
#include "fio.h"
#ifndef KR_headers
#undef abs
#undef min
#undef max
#include <stdlib.h>
#endif

#ifndef NON_POSIX_STDIO
#ifdef MSDOS
#include "io.h"
#else
#include "unistd.h"     /* for access */
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

 VOID
#ifdef KR_headers
g_char(a,alen,b) char *a,*b; ftnlen alen;
#else
g_char(char *a, ftnlen alen, char *b)
#endif
{
	char *x = a + alen, *y = b + alen;

	for(;; y--) {
		if (x <= a) {
			*b = 0;
			return;
			}
		if (*--x != ' ')
			break;
		}
	*y-- = 0;
	do *y-- = *x;
		while(x-- > a);
	}

 VOID
#ifdef KR_headers
b_char(a,b,blen) char *a,*b; ftnlen blen;
#else
b_char(char *a, char *b, ftnlen blen)
#endif
{	int i;
	for(i=0;i<blen && *a!=0;i++) *b++= *a++;
	for(;i<blen;i++) *b++=' ';
}
#ifndef NON_UNIX_STDIO
#ifdef KR_headers
long f__inode(a, dev) char *a; int *dev;
#else
long f__inode(char *a, int *dev)
#endif
{	struct STAT_ST x;
	if(STAT(a,&x)<0) return(-1);
	*dev = x.st_dev;
	return(x.st_ino);
}
#endif
#ifdef __cplusplus
}
#endif