#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t count = 0;
  while(s[count]!='\0'){
    count++;
  }
  return count;
}

char *strcpy(char *dst, const char *src) {
  char* ret=dst;
  while(*src != '\0')
  {
    *dst++=*src++;
  }
  *dst='\0';
  return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
  int i=0;
  for(;i<n && src[i]!= '\0';++i)
  {
    dst[i]=src[i];
  }
  for(;i<n;++i)
  {
    dst[i]='\0';
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  char* ret=dst;
  while(*dst){
    dst++;
  }
  while((*dst++ = *src ++));
  return ret;
}

int strcmp(const char *s1, const char *s2) {
  while(*s1 && *s1 == *s2)
  {
    s1++;s2++;
  };
  return (unsigned)*s1 - (unsigned) *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  int i=0;
  while(i<n && *s1 && *s1==*s2)
  {
    s1++;s2++;++i;
  }
  return (unsigned)*s1 - (unsigned) *s2;
}

void *memset(void *s, int c, size_t n) {
  char* p=s;
  for(int i=0;i<n;i++)
  {
    p[i] = (char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  char *d=dst;
  const char* s=src;
  if(d<s)
  {
    for(int i=0;i<n;++i)
    {
      d[i] = s[i];
    }
  }
  else{
    for(n=n-1;n>=0;--n)
    {
      d[n]=s[n];
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
    char *d = out;
    const char *s = in;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const char *p1=s1;
  const char *p2=s2;
  while(n--)
  {
    if(*p1 != *p2){
      return *p1-*p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

#endif
