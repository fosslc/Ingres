/*
 *  ap_config_auto.h -- Automatically determined configuration stuff
 *  THIS FILE WAS AUTOMATICALLY GENERATED - DO NOT EDIT!
 */

#ifndef AP_CONFIG_AUTO_H
#define AP_CONFIG_AUTO_H

/* check: #include <dlfcn.h> */
#ifdef HAVE_DLFCN_H
#undef HAVE_DLFCN_H
#endif

/* check: #include <dl.h> */
#ifndef HAVE_DL_H
#define HAVE_DL_H 1
#endif

/* check: #include <bstring.h> */
#ifdef HAVE_BSTRING_H
#undef HAVE_BSTRING_H
#endif

/* check: #include <crypt.h> */
#ifndef HAVE_CRYPT_H
#define HAVE_CRYPT_H 1
#endif

/* check: #include <unistd.h> */
#ifndef HAVE_UNISTD_H
#define HAVE_UNISTD_H 1
#endif

/* check: #include <sys/resource.h> */
#ifndef HAVE_SYS_RESOURCE_H
#define HAVE_SYS_RESOURCE_H 1
#endif

/* check: #include <sys/select.h> */
#ifdef HAVE_SYS_SELECT_H
#undef HAVE_SYS_SELECT_H
#endif

/* check: #include <sys/processor.h> */
#ifdef HAVE_SYS_PROCESSOR_H
#undef HAVE_SYS_PROCESSOR_H
#endif

/* determine: longest possible integer type */
#ifndef AP_LONGEST_LONG
#define AP_LONGEST_LONG long long
#endif

/* determine: byte order of machine (12: little endian, 21: big endian) */
#ifndef AP_BYTE_ORDER
#define AP_BYTE_ORDER 21
#endif

/* determine: is off_t a quad */
#ifndef AP_OFF_T_IS_QUAD
#undef AP_OFF_T_IS_QUAD
#endif

/* determine: is void * a quad */
#ifndef AP_VOID_P_IS_QUAD
#undef AP_VOID_P_IS_QUAD
#endif

/* build flag: -DHPUX10 */
#ifndef HPUX10
#define HPUX10 1
#endif

/* build flag: -DUSE_HSREGEX */
#ifndef USE_HSREGEX
#define USE_HSREGEX 1
#endif

/* build flag: -DUSE_EXPAT */
#ifndef USE_EXPAT
#define USE_EXPAT 1
#endif

/* build flag: -D_HPUX_SOURCE */
#ifndef _HPUX_SOURCE
#define _HPUX_SOURCE 1
#endif

#endif /* AP_CONFIG_AUTO_H */
