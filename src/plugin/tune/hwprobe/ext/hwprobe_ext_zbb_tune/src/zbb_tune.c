#include <limits.h>
#include <stdint.h>
#include <stddef.h>
typedef unsigned long __attribute((__may_alias__)) op_t;
typedef op_t find_t;

#define ALIGN_DOWN(base, size)	((base) & -((__typeof__ (base)) (size)))
#define PTR_ALIGN_DOWN(base, size) \
  ((__typeof__ (base)) ALIGN_DOWN ((uintptr_t) (base), (size)))
# define __always_inline __inline __attribute__ ((__always_inline__))
# define find_zero_low		find_zero_all

static __always_inline find_t find_zero_all(op_t x)
{
  find_t r;
  asm ("orc.b %0, %1" : "=r" (r) : "r" (x));
  return ~r;
}

static __always_inline find_t shift_find(find_t word, uintptr_t s)
{
  return word >> (CHAR_BIT * (s % sizeof (op_t)));
}

static __always_inline int ctz(find_t c)
{
  if (sizeof (find_t) == sizeof (unsigned long))
    return __builtin_ctzl (c);
  else
    return __builtin_ctzll (c);
}

static __always_inline unsigned int index_first(find_t c)
{
  int r;
  r = ctz (c);
  return r / CHAR_BIT;
}

static __always_inline _Bool has_zero(op_t x)
{
  return find_zero_low (x) != 0;
}

static __always_inline unsigned int index_first_zero(op_t x)
{
  x = find_zero_low (x);
  return index_first (x);
}

size_t strlen(const char *str)
{
  const uintptr_t s_int = (uintptr_t) str;
  const op_t *word_ptr = (const op_t*) PTR_ALIGN_DOWN (str, sizeof (op_t));

  op_t word = *word_ptr;
  find_t mask = shift_find (find_zero_all (word), s_int);
  if (mask != 0)
    return index_first (mask);

  do
    word = *++word_ptr;
  while (!has_zero(word));

  return ((const char *) word_ptr) + index_first_zero (word) - str;
}
