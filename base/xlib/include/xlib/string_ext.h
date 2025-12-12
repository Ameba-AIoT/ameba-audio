#ifndef AMEBA_AUDIO_BASE_XLIB_INCLUDE_XLIB_STRING_EXT_H
#define AMEBA_AUDIO_BASE_XLIB_INCLUDE_XLIB_STRING_EXT_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t xstrlcpy(char *dst, const char *src, size_t size);
size_t xstrlcat(char *dst, const char *src, size_t size);
size_t xstrnlen(const char *str, size_t maxlen);

int xstrcasecmp(const char *s1, const char *s2);
int xstrncasecmp(const char *s1, const char *s2, size_t n);

char *xstrdup(const char *str);

#ifdef __cplusplus
}
#endif

#endif // AMEBA_AUDIO_BASE_XLIB_INCLUDE_XLIB_STRING_EXT_H
