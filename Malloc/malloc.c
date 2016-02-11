/*
 TURN IN VERSION
 
 ANH NGUYEN
 version 1/15/2016
 */

#include "malloc.h"

/* head of the LinkedList of header */
header *headerList;
/* indicator of the break */
void *endOfHunk;

/* Mimic the library's malloc function */
void *malloc(size_t size) {
    
    /* indicate the special case of first attempt to malloc */
    static int firstCall = TRUE;
    /* contain address of the header */
    void *address;
    /* number of HUNKs that we need to sbrk */
    int hunkNeeded;
    /* error message */
    char str[MSG_SIZE];
    
    /* special case when requesting to size 0 */
    if (size == 0) {
        return NULL;
    }
    
    /* special case: first attempt to malloc */
    if (firstCall) {
        /* move the break by HUNK(s) if requested size too big */
        hunkNeeded = (HEADER_SPACE + (int)size - 1)/HUNK + 1;
        address = sbrk(hunkNeeded * HUNK);
        
        /* Unsuccesful moving the break */
        if (address == FAIL_BRK) {
            errno = ENOMEM;
            return NULL;
        }
        /* Successful moving the break */
        headerList = address;
        makeHeader(address, TRUE, headerList, headerList, (int)size);
        endOfHunk = address + HUNK * hunkNeeded;
        firstCall = FALSE;
        address += HEADER_SPACE;
        
        /* print debug error before return */
        /*
        if (getenv("DEBUG_MALLOC")) {
            sprintf(str, "MALLOC: malloc(ptr=%p, size=%zu\n");
            fputs(str, stderr);
        }*/
        return address;
    }
    
    /* general case */
    /* search for suitable free block in the list */
    address = searchBlock(headerList, (int)size, FALSE);
    
    /* not found a suitable free block: add new block */
    if (address == NULL) {
        address = ((void *)headerList->prev) + HEADER_SPACE
        + headerList->prev->size;
        /* if Hunk still have enough space: add new block to the Hunk */
        if (HEADER_SPACE + DATA_SPACE <= endOfHunk - address) {
            makeHeader(address, TRUE, headerList, headerList->prev, (int)size);
            headerList->prev->next = (header *)address;
            headerList->prev = (header *)address;
        }
        /* Hunk does not have enough space, sbrk() for more */
        else {
            int spaceLeft = (int)(endOfHunk - address);
            hunkNeeded = (HEADER_SPACE + (int)size - 1 - spaceLeft) /HUNK + 1 ;
            address = sbrk(hunkNeeded * HUNK);
            
            /* Unsuccesful moving the break */
            if (address == FAIL_BRK) {
                errno = ENOMEM;
                return NULL;
            }
            /* Successful moving the break */
            endOfHunk = address + HUNK * hunkNeeded;
            address = address - spaceLeft;
            makeHeader(address, TRUE, headerList, headerList->prev, (int)size);
            headerList->prev->next = (header *)address;
            headerList->prev = (header *)address;
        }
    }
    /* found a suitable free block: use that block */
    else if ( ((header *)address)->size >= DATA_SPACE ){
        /* if the found block is too big, split it into 2 and use the first */
        splitBlock(address, (int)size);
    }
    /* calculate return address */
    address = address + HEADER_SPACE;
    /* print debug error message */
    /*
    if (getenv("DEBUG_MALLOC")) {
        sprintf(str, "MALLOC: malloc(ptr%p, size=%zu\n", address, size);
        fputs(str, stderr);
    }*/
    return address;
}

/* A helper function to make header
 @return the pointer to the newly created header. */
header *makeHeader(void *address,
                   int isUsed, struct header *next, struct header *prev, int size) {
    header *temp = address;
    
    temp->isUsed = TRUE;
    temp->next = next;
    temp->prev = prev;
    temp->size = DATA_SPACE;
    
    return address;
}

/* A helper function to search for a block with the state of used, and size
 @return NULL if not found any suitable Block. */
header *searchBlock(void *ptr, int size, int isUsed) {
    header *temp = headerList; /* pointer to traverse the list */
    header *stop = NULL;       /* stop traversing when reach this */
    void *low;                 /* low bound of internal pointer */
    void *high;                /* high bound of internal pointer */
    
    /* obvious case when ptr is not in my HUNK */
    if(ptr < (void *)headerList || ptr > endOfHunk - HEADER_SPACE) {
        return NULL;
    }
    /* search for a used block which was point to by ptr to free */
    if (isUsed == TRUE) {
        /* traverse the linkedlist to find the block which was point to by ptr*/
        while( (void *)temp < ptr && temp != stop) {
            stop = headerList;
            low = ((void *)temp) + HEADER_SPACE;
            high = ((void *)temp) + HEADER_SPACE + temp->size;
            if (ptr >= low && ptr <= high){
                return temp;
            }
            temp = temp->next;
        }
        /* check last block on the list */
        low = headerList->prev + HEADER_SPACE;
        high = low + headerList->prev->size;
        if (ptr >= low && ptr <= high){
            return temp;
        }
        /* not found, return NULL */
        return NULL;
        
    }
    /* search for a free block with size > requested size */
    while(temp != stop) {
        stop = headerList;
        if(temp->isUsed == isUsed && temp->size >= size)
            return temp;
        else
            temp = temp->next;
    }
    return NULL;
}

/* A helper function to split the Block into 2.
 1st block will be use, 2nd block will be mark as unused
 @param size will be the size of the first newly Block. */
void splitBlock(header *head, int size) {
    header *second = head + HEADER_SPACE + size;
    /* if sufficient space to split */
    if (head->size - DATA_SPACE >= HEADER_SPACE) {
        /* set fields of 2nd block */
        makeHeader(second, FALSE, head->next, head, size);
        second->size = head->size - HEADER_SPACE - DATA_SPACE;
        /* change fields of 1st block */
        makeHeader(head, TRUE, second, head->prev, size);
    }
    else {
        makeHeader(head, TRUE, head->next, head->prev, size);
    }
}

/* Mimic the library's funtion calloc */
void *calloc(size_t nmemb, size_t size) {
    /* use malloc to allocate memory on the HUNK */
    void *address = malloc(nmemb * size);
    void *step = address;
    char str[MSG_SIZE];
    /* wipe out all the junk */
    while (step - address < nmemb * size) {
        *((char*)step++) = 0;
    }
    /*
    if (getenv("DEBUG_MALLOC")) {
        sprintf(str, "MALLOC: calloc(ptr=%p, size=%zu\n",
                address, nmemb * size);
        fputs(str, stderr);
    }*/
    return address;
}

/* Mimic the library's funtion free. However, argument ptr is not required to
 point to the first byte of the region, just need to point to that region */
void free(void *ptr) {
    header *temp;
    char str[MSG_SIZE];
    
    /* special case when ptr was NULL, or not on my HUNK, do nothing */
    if (ptr == NULL || ptr < (void *)headerList + HEADER_SPACE ||
        ptr > endOfHunk - headerList->prev->size) {
        return;
    }
    /* print debug message */
    /*
    if (getenv("DEBUG_MALLOC")) {
        sprintf(str, "MALLOC: malloc(%p)\n", ptr);
        fputs(str, stderr);
    }*/
    /* search for the block of memory pointed by ptr */
    temp = searchBlock(ptr, 0, TRUE);
    /* found the block of memory, set isUsed field to false */
    if (temp != NULL) {
        ((header *)temp)->isUsed = FALSE;
    }
}

/* Mimic the library's function realloc */
void *realloc(void *ptr, size_t size) {
    header *temp;
    int oldSize;        /* old size of the block pointed by ptr */
    void *address;      /* the address that will be return */
    char str[MSG_SIZE];
    
    /* special case: ptr is NULL, return malloc */
    if (ptr == NULL) {
        return malloc(size);
    }
    /* special case: ptr is not NULL and requested size equal 0, free(ptr) */
    else if (size == 0) {
        free(ptr);
        return NULL;
    }
    
    /* if ptr is a valid pointer was created by malloc, calloc, or realloc */
    if ((temp = searchBlock(ptr, 0, TRUE)) != NULL) {
        /* initialize variables */
        oldSize = temp->size;
        address = ptr;
        
        /* general case 1: requested size is smaller or equal to old size*/
        if (temp->size >= size) {
            splitBlock(temp, (int)size);
            return ptr;
        }
        /* general case 2: requested size is larger than old size (expanding)*/
        else {
            /* if it was last block & HUNK still have enough space: expand it*/
            if (temp->next == headerList &&
                (endOfHunk - (void*)headerList->prev) > (size - oldSize)) {
                temp->size = (int)size;
                return ptr;
            }
            /* adjt blocks are not free, or still not enough, allocating new*/
            if ((address = malloc(size)) != NULL) {
                free(ptr);
                makeHeader(address, TRUE, headerList,
                           headerList->prev, (int)size);
                headerList->prev->next = address;
                headerList->prev = address;
                memmove(address + HEADER_SPACE, ptr, oldSize);
                address = address + HEADER_SPACE;
            }
        }
    }
    /* invalid ptr pointer, return NULL */
    else {
        address = NULL;
    }
    /* print debug message */
    /*
    if (getenv("DEBUG_MALLOC")) {
        sprintf(str, "MALLOC: realloc(ptr=%p, size=%zu\n)", address, size);
        fputs(str, stderr);
    }*/
    return address;
}

int main(int argc, char *argv[]){
    unsigned char *val;
    
    val = malloc (1024);
    if ( val )
        printf("Calling malloc succeeded.\n");
    else {
        printf("malloc() returned NULL.\n");
        exit(1);
    }
    
    fill(val,1024,0);
    printf("Successfully used the space.\n");
    
    if ( check(val,1024,0) )
        printf("Some values didn't match in region %p.\n",val);
    
    printf("Allocated 1024 bytes.  Freeing val+512.\n");
    free(val+512);                /* free from the middle */
    printf("We survived the free call.\n");
    
    exit(0);
}

