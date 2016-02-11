#include<stdlib.h>

void fill(unsigned char *s, size_t size, int start) {
    int i;
    for(i=0; i< size; i++)
        s[i]=start++;
}

int check(unsigned char *s, size_t size, int start){
    int i,err=0;
    for(i=0; i < size; i++,start++)
        err += (s[i] != (start&0xff));
    
    return err;
}
