#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define TRUE 1
#define FALSE 0
#define HUNK 64000
#define ALIGNMENT 16
#define FAIL_BRK ((void *)-1)
/* convert the size of header into multiple of ALIGNMENT */
#define HEADER_SPACE ((sizeof(header) - 1)/ALIGNMENT * ALIGNMENT + ALIGNMENT)
/* convert data space to multiple of ALIGNMENT */
#define DATA_SPACE (((int)size - 1)/ALIGNMENT * ALIGNMENT + ALIGNMENT)
#define MSG_SIZE 40

typedef struct header
{
    int index;           /* represent the location order of the block on the memory */
    int size;            /* size of data space, exclude header's space */
    int isUsed;          /* indicate if a block of memory is in use or not */
    struct header *next; /* next block of memory on the memory */
    struct header *prev; /* prev block of memory on the memory */
} header;

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

header *makeHeader(void *address, int isUsed, struct header *next, struct header *prev, int size);
header *searchBlock(void *head, int size, int isUsed);
void splitBlock(header *head, int size);