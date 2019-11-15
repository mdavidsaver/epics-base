#ifndef LIBCOMAPI_H
#define LIBCOMAPI_H

#if defined(_WIN32) || defined(__CYGWIN__)
#   define LIBCOMSTD_API __stdcall

#  if defined(LIBCOM_API_BUILDING) && defined(EPICS_BUILD_DLL)
/* building library as dll */
#    define LIBCOM_API __declspec(dllexport)
#  elif !defined(LIBCOM_API_BUILDING) && defined(EPICS_CALL_DLL)
/* calling library in dll form */
#    define LIBCOM_API __declspec(dllimport)
#  endif

#elif __GNUC__ >= 4
#  define LIBCOM_API __attribute__ ((visibility("default")))
#endif

#if !defined(LIBCOM_API)
#  define LIBCOM_API
#endif

#if !defined(LIBCOMSTD_API)
#  define LIBCOMSTD_API
#endif

#endif /* LIBCOMAPI_H */
