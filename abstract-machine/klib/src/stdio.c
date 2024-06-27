#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...)
{
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
  panic("Not implemented");
}
static void itoa(int n, char *s)
{
  int i, sign;
  if ((sign = n) < 0)
    n = -n;
  i = 0;
  do
  {
    s[i++] = n % 10 + '0';
  } while ((n /= 10) > 0);
  if (sign < 0)
    s[i++] = '-';
  s[i] = '\0';
  for (int j = 0, k = i - 1; j < k; j++, k--)
  {
    char temp = s[j];
    s[j] = s[k];
    s[k] = temp;
  }
}
int sprintf(char *out, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int count = 0;
  for (const char *p = fmt; *p; p++)
  {
    if (*p == '%')
    {
      p++;
      switch (*p)
      {
      case 'd':
      {
        int num = va_arg(args, int);
        char numStr[20];
        itoa(num, numStr);
        for (char *ptr = numStr; *ptr; ptr++, out++)
        {
          *out = *ptr;
          count++;
        }
        break;
      }
      case 's':
      {
        char *str = va_arg(args, char *);
        while (*str)
        {
          *out++ = *str++;
          count++;
        }
        break;
      }
      default:
        *out++ = '?';
        count++;
        break;
      }
    }
    else
    {
      *out++ = *p;
      count++;
    }
  }
  *out = '\0';
  va_end(args);
  return count;
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
  panic("Not implemented");
}

#endif
