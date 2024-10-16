/*************************************************************************\
* Copyright (c) 2009 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/**
 * \file epicsStdio.h
 * \brief Standardize the behavior of stdio across EPICS targets
 *
 * \details
 * The `epicsStdio.h` header includes the operating system's `stdio.h` header
 * and if used should replace it.
 *
 * epicsSnprintf() and epicsVsnprintf() have the same semantics as the C99
 * functions snprintf() and vsnprintf(), but correct the behavior of the
 * implementations on some operating systems.
 *
 * @note Define `epicsStdioStdStreams` and/or `epicsStdioStdPrintfEtc`
 *       to opt out of the redirection described below.
 *
 * The routines epicsGetStdin(), epicsGetStdout(), epicsGetStderr(),
 * epicsStdoutPrintf(), epicsStdoutPuts(), and epicsStdoutPutchar()
 * are not normally named directly in user code. They are provided for use by
 * various macros which redefine several standard C identifiers.
 * This is done so that any I/O through these standard streams can be
 * redirected, usually to or from a file. The primary use of this facility
 * is for iocsh() and any commands called from it, allowing redirection of
 * the standard input and/or output streams while running those commands.
 * In order for this redirection to work, all modules involved in such I/O
 * must include `epicsStdio.h` instead of the system header `stdio.h`.
 * The redirections are:
 * - `stdin` becomes epicsGetStdin()
 * - `stdout` becomes epicsGetStdout()
 * - `stderr` becomes epicsGetStderr()
 * - `printf` becomes epicsStdoutPrintf()
 * - `puts` becomes epicsStdoutPuts()
 * - `putchar` becomes epicsStdoutPutchar()
 * - `vprintf` becomes epicsStdoutVPrintf()
 *
 * The epicsSetThreadStdin(), epicsSetThreadStdout() and epicsSetThreadStderr()
 * routines allow the standard file streams to be redirected on a per thread
 * basis, e.g. calling epicsThreadStdout() will affect only the thread which
 * calls it. To cancel a stream redirection, pass a NULL argument in another
 * call to the same redirection routine that was used to set it.
 *
 * @since 3.15.6 define `epicsStdioStdPrintfEtc` to opt out of redefinition
 *        for `printf`, `vprintf`, `puts`, and `putchar`.
 *
 * @since 3.15.0 define `epicsStdioStdStreams` to opt out of redefinition
 *        of `stdin`, `stdout`, and `stderr`.
 */

#ifndef epicsStdioh
#define epicsStdioh

#include <stdio.h>
#include <stdarg.h>

#include "libComAPI.h"
#include "epicsTypes.h"
#include "compilerDependencies.h"
#include "epicsTempFile.h"

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef epicsStdioStdStreams
#  undef stdin
#  define stdin epicsGetStdin()
#  undef stdout
#  define stdout epicsGetStdout()
#  undef stderr
#  define stderr epicsGetStderr()
#endif

/* Make printf, puts and putchar use *our* version of stdout */

#ifndef epicsStdioStdPrintfEtc
#  ifdef printf
#    undef printf
#  endif
#  define printf epicsStdoutPrintf

#  ifdef vprintf
#    undef vprintf
#  endif
#  define vprintf epicsStdoutVPrintf

#  ifdef puts
#    undef puts
#  endif
#  define puts epicsStdoutPuts

#  ifdef putchar
#    undef putchar
#  endif
#  define putchar epicsStdoutPutchar
#endif

/**
 * \brief epicsSnprintf() is meant to have the same semantics as the C99
 * function snprintf()
 *
 * \details
 * This is provided because some architectures do not implement these functions,
 * while others implement them incorrectly.
 * Standardized as a C99 function, snprintf() acts like sprintf() except that
 * the `size` argument gives the maximum number of characters (including the
 * trailing zero byte) that may be placed in `str`.
 *
 * On some operating systems though the implementation of this function does
 * not always return the correct value. If the OS implementation does not
 * correctly return the number of characters that would have been written when
 * the output gets truncated, it is not worth trying to fix this as long as
 * they return `size-1` instead; the resulting string must always be correctly
 * terminated with a zero byte.
 *
 * In some scenarios the epicsSnprintf() implementation may not provide the
 * correct C99 semantics for the return value when `size` is given as zero.
 * On these systems epicsSnprintf() can return an error (a value less than
 * zero) when a buffer length of zero is passed in, so callers should not use
 * that technique to calculate the length of the buffer required.
 *
 * \return The number of characters (not counting the terminating zero byte)
 * that would be written to `str` if it was large enough to hold them all; the
 * output has been truncated if the return value is `size` or more.
 */
LIBCOM_API int epicsStdCall epicsSnprintf(
    char *str, size_t size, EPICS_PRINTF_FMT(const char *format), ...
) EPICS_PRINTF_STYLE(3,4);
/**
 * \brief epicsVsnprintf() is meant to have the same semantics as the C99
 * function vsnprintf()
 *
 * \details
 * This is provided because some architectures do not implement these functions,
 * while others implement them incorrectly.
 * Standardized as a C99 function, vsnprintf() acts like vsprintf() except that
 * the `size` argument gives the maximum number of characters (including the
 * trailing zero byte) that may be placed in `str`.
 *
 * On some operating systems though the implementation of this function does
 * not always return the correct value. If the OS implementation does not
 * correctly return the number of characters that would have been written when
 * the output gets truncated, it is not worth trying to fix this as long as
 * they return `size-1` instead; the resulting string must always be correctly
 * terminated with a zero byte.
 *
 * In some scenarios the epicsSnprintf() implementation may not provide the
 * correct C99 semantics for the return value when `size` is given as zero.
 * On these systems epicsSnprintf() can return an error (a value less than
 * zero) when a buffer length of zero is passed in, so callers should not use
 * that technique to calculate the length of the buffer required.
 *
 * \return The number of characters (not counting the terminating zero byte)
 * that would be written to `str` if it was large enough to hold them all; the
 * output has been truncated if the return value is `size` or more.
 */
LIBCOM_API int epicsStdCall epicsVsnprintf(
    char *str, size_t size, const char *format, va_list ap);

enum TF_RETURN {TF_OK=0, TF_ERROR=1};
/**
 * \brief Truncate a file to a specified size
 * \note The BSD function truncate() was not available on all targets
 * \param pFileName Name (and optionally path) of file
 * \param size The new file size (if files is currently larger)
 * \return `TF_OK` if the file is less that size bytes or if it was successfully
 * truncated; `TF_ERROR` if the file could not be truncated
 */
LIBCOM_API enum TF_RETURN truncateFile ( const char *pFileName, unsigned long size );

/* The following are for redirecting stdin,stdout,stderr */
LIBCOM_API FILE * epicsStdCall epicsGetStdin(void);
LIBCOM_API FILE * epicsStdCall epicsGetStdout(void);
LIBCOM_API FILE * epicsStdCall epicsGetStderr(void);
/* These are intended for iocsh only */
LIBCOM_API FILE * epicsStdCall epicsGetThreadStdin(void);
LIBCOM_API FILE * epicsStdCall epicsGetThreadStdout(void);
LIBCOM_API FILE * epicsStdCall epicsGetThreadStderr(void);
LIBCOM_API void  epicsStdCall epicsSetThreadStdin(FILE *);
LIBCOM_API void  epicsStdCall epicsSetThreadStdout(FILE *);
LIBCOM_API void  epicsStdCall epicsSetThreadStderr(FILE *);

LIBCOM_API int epicsStdCall epicsStdoutPrintf(
    const char *pformat, ...) EPICS_PRINTF_STYLE(1,2);
LIBCOM_API int epicsStdCall epicsStdoutVPrintf(
    const char *pformat, va_list ap);
LIBCOM_API int epicsStdCall epicsStdoutPuts(const char *str);
LIBCOM_API int epicsStdCall epicsStdoutPutchar(int c);

/**
 * \brief Open file from memory or disk.
 * \param pathname Path of file to open.
 * \param mode Open Mode.  For a memory file only "r" or "rb" are accepted.
 * \returns NULL on error and sets `errno`
 *
 * If `pathname` is prefixed with `app://`, then an entry
 * previously passed to epicsFMount() may be returned.
 * Otherwise, the call is forwarded to the OS fopen().
 *
 * \note `app://` files can only be opened for read-only access,
 *       and may not have an associated file descriptor.
 *
 * \note `app://` is not available on vxWorks.
 *
 * \since UNRELEASED
 */
LIBCOM_API
FILE* epicsFOpen(const char *pathname, const char *mode);

/**
 * \brief Open file memory file.
 * \param pathname Path of file to open.
 * \param mode Open Mode.  Only "r" or "rb" are accepted.
 * \returns NULL on error and sets `errno`
 *
 * `pathname` must not be prefixed with `app://`.
 *
 * \since UNRELEASED
 */
LIBCOM_API
FILE* epicsMemOpen(const char *pathname, const char *mode);

typedef enum {
    /** Content array is raw bytes (uncompressed) */
    epicsIMFRaw,
    /** Content array is zlib compressed */
    epicsIMFZ,
} epicsIMFEncoding;

/** \brief Information about a memory file
 */
typedef struct epicsIMF {
    /** Absolute path with which this file should be associated
     *  under the `app://` in-memory file system.
     */
    const char *path;
    /** \brief File contents */
    const char *content;
    /** \brief Length of file contents in bytes */
    size_t contentLen;
    /** \brief Actual/uncompressed content length in bytes.
     *
     * If `encoding==epicsIMFRaw`
     * then `contentLen==actualLen`.
     * Otherwise, `contentLen` is the compressed size,
     * and `actualLen` is the uncompressed size.
     *
     * @warning `contentLen==actualLen` does not imply uncompressed data!
     */
    size_t actualLen;
    /** Indicate how the content array is encoded. */
    epicsIMFEncoding encoding;
    /** Request that `epicsMemMount()` not copy epicsIMF::content .
     *  This array pointed to must never be `free()`d.
     *  An optimization when the content array is a compile time constant.
     */
    unsigned nocopy:1;
} epicsIMF;

/** Print extended error information to `epicsGetStderr()`. */
#define EPICS_MEM_MOUNT_VERBOSE (0x100)

/**
 * \brief Attach memory files
 * \param files Base of an array of `epicsIMF`.
 *              The last element must have a NULL path.
 * \param flags bitwise "or" of zero or more of `EPICS_MEM_MOUNT_*`.
 * \return zero if all files were successfully mounted.
 *         non-zero if any file could not be mounted.
 *         (pass `flags = EPICS_MEM_MOUNT_VERBOSE` for details)
 *
 * Attach some files to the in-memory `app://` file system.
 *
 * The `epicsIMF` structure and the path string will always be copied,
 * and need not remain valid after this function returns.
 * If `epicsIMF::nocopy` is set, then the `epicsIMF::content`
 * array must remain valid in perpetuity.  Otherwise, `content`
 * is copied as well.
 *
 * \since UNRELEASED
 */
LIBCOM_API
int epicsMemMount(const epicsIMF* files,
                  epicsUInt64 flags);

/**
 * \brief Detach a memory file
 * \param path One of the path strings passed through epicsMemMount()
 * \param flags Currently unused.  Set to zero.
 * \return zero on success
 */
LIBCOM_API
int epicsMemUnmount(const char *path, epicsUInt64 flags);

LIBCOM_API
void epicsMemFileShow(void);

#ifdef EPICS_PRIVATE_API

/**
 * \brief Iteration of memory files
 * \param func Callback function pointer
 * \param arg Passed to callback function
 *
 * The provided callback will be called for each of the currently
 * mounted memory files.
 *
 * \note Callbacks are made with an internal local held which
 *       prevents concurrent operations on memory files.
 *       Callbacks may not call epicsMemMount() or epicsMemUnmount()
 */
LIBCOM_API
void epicsMemFileForEach(void (*func)(void *, const epicsIMF *),
                         void *arg);

#endif /* EPICS_PRIVATE_API */

#ifdef  __cplusplus
}

/* Also pull functions into the std namespace (see lp:1786927) */
#if !defined(__GNUC__) || (__GNUC__ > 2)
namespace std {
using ::epicsGetStdin;
using ::epicsGetStdout;
using ::epicsGetStderr;
using ::epicsStdoutPrintf;
using ::epicsStdoutVPrintf;
using ::epicsStdoutPuts;
using ::epicsStdoutPutchar;
}
#endif /* __GNUC__ > 2 */

#endif /* __cplusplus */

#endif /* epicsStdioh */
