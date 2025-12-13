#ifndef AMEBA_AUDIO_BASE_XLIB_INCLUDE_XLIB_STDIO_EXT_H
#define AMEBA_AUDIO_BASE_XLIB_INCLUDE_XLIB_STDIO_EXT_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int xvasprintf(char **ptr, const char *format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif // AMEBA_AUDIO_BASE_XLIB_INCLUDE_XLIB_STDIO_EXT_H
