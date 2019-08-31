#ifndef _SYSTEM_H
#define _SYSTEM_H 1

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>


#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
  ({ ssize_t __res; \
     do \
       __res = expression; \
     while (__res == -1 && errno == EINTR); \
     __res; })
#endif

static inline ssize_t __attribute__ ((unused))
pwrite_retry (int fd, const void *buf, size_t len, off_t off)
{
  ssize_t recvd = 0;

  do
    {
      ssize_t ret = TEMP_FAILURE_RETRY (pwrite (fd, buf + recvd, len - recvd,
                                                off + recvd));
      if (ret <= 0)
        return ret < 0 ? ret : recvd;

      recvd += ret;
    }
  while ((size_t) recvd < len);

  return recvd;
}

static inline ssize_t __attribute__ ((unused))
pread_retry (int fd, void *buf, size_t len, off_t off)
{
  ssize_t recvd = 0;

  do
    {
      ssize_t ret = TEMP_FAILURE_RETRY (pread (fd, buf + recvd, len - recvd,
                                               off + recvd));
      if (ret <= 0)
        return ret < 0 ? ret : recvd;

      recvd += ret;
    }
  while ((size_t) recvd < len);

  return recvd;
}

#endif /* _SYSTEM_H */
