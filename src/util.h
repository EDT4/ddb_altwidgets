#ifndef __DDB_ALTERNATIVEWIDGETS_UTIL_H
#define __DDB_ALTERNATIVEWIDGETS_UTIL_H

#include <stddef.h>
#include <stdint.h>

uint32_t parse_u32(char const **s);
uint32_t *parse_u32s(char const **s,uint32_t out[],size_t out_size,char delim);
size_t write_u32(uint32_t n,char *out,size_t out_size);
size_t write_u32s(uint32_t ns[],size_t size,char delim,char *out,size_t out_size);

#endif
