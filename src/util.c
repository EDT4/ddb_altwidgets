#include "util.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//`s` and `*s` should not be NULL.
//Returns the parsed integer or 0 with `*s` unchanged for parsing errors.
uint32_t parse_u32(char const **s){
	uint32_t out = 0;
	while('0' <= **s && **s <= '9'){
		out*= 10;
		out+= *(*s)++ - '0';
	}
	return out;
}

//`s` and `*s` should not be NULL.
//`out` can only be NULL if `out_size` is 0.
//`delim` should not be a digit or \0.
//Returns a pointer to the integer after the last written to `out`.
uint32_t *parse_u32s(char const **s,uint32_t out[],size_t out_size,char delim){
	if(**s != '\0') while(out_size-->0){
		*out++ = parse_u32(s);
		if(**s == delim){
			(*s)+= 1;
		}else{
			break;
		}
	}
	return out;
}

inline static size_t write_str(const char *str,size_t len,char *out,size_t out_size){
	if(len > out_size) return 0;
	memcpy(out,str,len);
	return len;
}

//`out` should not be NULL.
//Returns the number of bytes that was written to `out`.
//The written string will not be NULL-terminated.
//Nothing will be written iff `out_size` is too small.
size_t write_u32(uint32_t n,char *out,size_t out_size){
	char buf[(sizeof(uint32_t)*CHAR_BIT)/3 + ((sizeof(uint32_t)*CHAR_BIT)%3 > 0)]; //Size should be ceil(log10(2^(sizeof(uint32_t)*8))) = ceil(sizeof(uint32_t)*8/log2(10)) but an estimation of an upper bound is used here instead.
	char *p = buf + sizeof(buf);
	do{
		*--p = '0' + n%10;
		n/= 10;
	}while(n > 0);
	return write_str(p,(buf + sizeof(buf)) - p,out,out_size);
}

//`ns` should not be NULL.
//`out` should not be NULL.
//The written string will not be NULL-terminated.
//Returns the number of bytes that was written to `out`, or 0 if `out_size` is too small.
size_t write_u32s(uint32_t ns[],size_t size,char delim,char *out,size_t out_size){
	size_t len = 0;
	if(size-->0){
		Loop: {
			size_t l = write_u32(*ns++,out+len,out_size-len);
			if(l == 0) return 0;
			len+= l;

			if(size-->0 && len < out_size){
				out[len++] = delim;
				goto Loop;
			}
		}
	}
	return len;
}
