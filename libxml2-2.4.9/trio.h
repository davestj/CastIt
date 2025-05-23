/*************************************************************************
 *
 * $Id: trio.h,v 1.1 2004/06/21 22:50:58 oddsock Exp $
 *
 * Copyright (C) 1998 Bjorn Reese and Daniel Stenberg.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS AND
 * CONTRIBUTORS ACCEPT NO RESPONSIBILITY IN ANY CONCEIVABLE MANNER.
 *
 ************************************************************************/

#ifndef TRIO_TRIO_H
#define TRIO_TRIO_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/*
 * Use autoconf defines if present. Packages using trio must define
 * HAVE_CONFIG_H as a compiler option themselves.
 */
#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif

#if !defined(WITHOUT_TRIO)

#ifdef __cplusplus
extern "C" {
#endif

/* make utility and C++ compiler in Windows NT fails to find this symbol */ 
#if defined(WIN32) && !defined(isascii)
# define isascii ((unsigned)(x) < 0x80)
#endif

/*
 * Error codes.
 *
 * Remember to add a textual description to trio_strerror.
 */
enum {
  TRIO_EOF      = 1,
  TRIO_EINVAL   = 2,
  TRIO_ETOOMANY = 3,
  TRIO_EDBLREF  = 4,
  TRIO_EGAP     = 5,
  TRIO_ENOMEM   = 6,
  TRIO_ERANGE   = 7
};

/* Error macros */
#define TRIO_ERROR_CODE(x) ((-(x)) & 0x00FF)
#define TRIO_ERROR_POSITION(x) ((-(x)) >> 8)
#define TRIO_ERROR_NAME(x) trio_strerror(x)

const char *trio_strerror(int);

/*************************************************************************
 * Print Functions
 */

int trio_printf(const char *format, ...);
int trio_vprintf(const char *format, va_list args);
int trio_printfv(const char *format, void **args);

int trio_fprintf(FILE *file, const char *format, ...);
int trio_vfprintf(FILE *file, const char *format, va_list args);
int trio_fprintfv(FILE *file, const char *format, void **args);

int trio_dprintf(int fd, const char *format, ...);
int trio_vdprintf(int fd, const char *format, va_list args);
int trio_dprintfv(int fd, const char *format, void **args);

/* trio_sprintf(target, format, ...)
 * trio_snprintf(target, maxsize, format, ...)
 *
 *   Build 'target' according to 'format' and succesive
 *   arguments. This is equal to the sprintf() and
 *   snprintf() functions.
 */
int trio_sprintf(char *buffer, const char *format, ...);
int trio_vsprintf(char *buffer, const char *format, va_list args);
int trio_sprintfv(char *buffer, const char *format, void **args);

int trio_snprintf(char *buffer, size_t max, const char *format, ...);
int trio_vsnprintf(char *buffer, size_t bufferSize, const char *format,
		   va_list args);
int trio_snprintfv(char *buffer, size_t bufferSize, const char *format,
		   void **args);

int trio_snprintfcat(char *buffer, size_t max, const char *format, ...);
int trio_vsnprintfcat(char *buffer, size_t bufferSize, const char *format,
                      va_list args);

char *trio_aprintf(const char *format, ...);
char *trio_vaprintf(const char *format, va_list args);

int trio_asprintf(char **ret, const char *format, ...);
int trio_vasprintf(char **ret, const char *format, va_list args);

/*************************************************************************
 * Scan Functions
 */
int trio_scanf(const char *format, ...);
int trio_vscanf(const char *format, va_list args);
int trio_scanfv(const char *format, void **args);

int trio_fscanf(FILE *file, const char *format, ...);
int trio_vfscanf(FILE *file, const char *format, va_list args);
int trio_fscanfv(FILE *file, const char *format, void **args);

int trio_dscanf(int fd, const char *format, ...);
int trio_vdscanf(int fd, const char *format, va_list args);
int trio_dscanfv(int fd, const char *format, void **args);

int trio_sscanf(const char *buffer, const char *format, ...);
int trio_vsscanf(const char *buffer, const char *format, va_list args);
int trio_sscanfv(const char *buffer, const char *format, void **args);

/*************************************************************************
 * Renaming
 */
#ifdef TRIO_REPLACE_STDIO
/* Replace the <stdio.h> functions */
#ifndef HAVE_PRINTF
# define printf trio_printf
#endif
#ifndef HAVE_VPRINTF
# define vprintf trio_vprintf
#endif
#ifndef HAVE_FPRINTF
# define fprintf trio_fprintf
#endif
#ifndef HAVE_VFPRINTF
# define vfprintf trio_vfprintf
#endif
#ifndef HAVE_SPRINTF
# define sprintf trio_sprintf
#endif
#ifndef HAVE_VSPRINTF
# define vsprintf trio_vsprintf
#endif
#ifndef HAVE_SNPRINTF
# define snprintf trio_snprintf
#endif
#ifndef HAVE_VSNPRINTF
# define vsnprintf trio_vsnprintf
#endif
#ifndef HAVE_SCANF
# define scanf trio_scanf
#endif
#ifndef HAVE_VSCANF
# define vscanf trio_vscanf
#endif
#ifndef HAVE_FSCANF
# define fscanf trio_fscanf
#endif
#ifndef HAVE_VFSCANF
# define vfscanf trio_vfscanf
#endif
#ifndef HAVE_SSCANF
# define sscanf trio_sscanf
#endif
#ifndef HAVE_VSSCANF
# define vsscanf trio_vsscanf
#endif
/* These aren't stdio functions, but we make them look similar */
#define dprintf trio_dprintf
#define vdprintf trio_vdprintf
#define aprintf trio_aprintf
#define vaprintf trio_vaprintf
#define asprintf trio_asprintf
#define vasprintf trio_vasprintf
#define dscanf trio_dscanf
#define vdscanf trio_vdscanf
#endif

/* strio compatible names */
#define StrScan trio_sscanf
#define StrFormat trio_sprintf
#define StrFormatMax trio_snprintf
#define StrFormatAlloc trio_aprintf
#define StrFormatAppendMax trio_snprintfcat

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WITHOUT_TRIO */

#endif /* TRIO_TRIO_H */
