#ifndef  CAPRINTF_H
#define  CAPRINTF_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <stdarg.h>


#ifdef WIN32
#define _FARARG_
#undef  FAR
#define FAR
#define _fstrcat    strcat
#define _fstrchr    strchr
#define _fstrlen    strlen
#else
#undef  FAR
#define FAR  far
#endif

/*
===========================================================================
     PAD and TOINT both have bad side effects so be careful.

        PAD(fw,fillchar,out,op)    outputs fillchar fw times. fw = 0.
        TOINT(p,x)                 works like "x = atoi(P)" except p is
                                   advanced past the number.
        INTMASK                    portable way to mask off the top N bits
                                   of a long, where N is the width of an int.
===========================================================================
*/

#define  PAD(fw,fc,out,op)    while( --(fw) >= 0 )   \
                                 (*out)( fc, op )    

#define  TOINT(p,x)           while( '0' <= *p && *p <= '9' )   \
                                 x = (x * 10) + (*p++ - '0')

#define  INTMASK              (long)( (unsigned)(~0) )

/*int  ca_printf(char  *fmt,...);*/
/*int  ca_fprintf(struct  _iobuf *stream,char  *fmt,...);*/
/*int  CAsprintf(char  *buf,char  *fmt,...);*/
/*void  idoprnt(int  (*out)(int, FILE *),void  *o_param,char  *format,char  *args);*/
/*char  *ltos(unsigned long  n,char  *buf,int  base);*/
/*char  *ftos(double  num,char  *buf,int  pre);*/
/*char  *etos(double  num,char  *buf,int  pre,char  fmt);*/

int  ca_printf(char _FARARG_ *fmt,...);
int  ca_fprintf(FILE _FARARG_ *stream,char _FARARG_ *fmt,...);
int  CAsprintf(char _FARARG_ *buf,char _FARARG_ *fmt,...);
#ifdef WIN32
  void  idoprnt(int  (*out)(int, char _FARARG_ * _FARARG_*),void _FARARG_ *o_param,char _FARARG_ *format,va_list args);
#else
  void  idoprnt(int  (*out)(int, char _FARARG_ * _FARARG_*),void _FARARG_ *o_param,char _FARARG_ *format,char _FARARG_ *args);
#endif
char  _FARARG_ *ltos(unsigned long  n,char _FARARG_ *buf,int  base);
char  _FARARG_ *ftos(double  num,char _FARARG_ *buf,int  pre);
char  _FARARG_ *etos(double  num,char _FARARG_ *buf,int  pre,char  fmt);

#endif  // wrapper
