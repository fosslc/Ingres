/****************************************************************************
 * CA_PRNTF.C
 *
 * This file has been modified for use within MS Windows.
 * Only CAsprintf is useful (or necessary) in this environment. 
 * The other 2 functions (ca_printf and ca_fprintf) have been commented out.
 ****************************************************************************/

/****************************************************************************
 * Module ca_prntf:  Shell for printf family - printf, sprintf, & fprintf.
 *
 * Initially needed because certain functions (like sprintf) couldn't be used 
 * in Windows dynamic link libraries (DLLs). Only functions which assume that
 * DS != SS can be used in DLLs. DLLs have their own data segment but use
 * the stack segment of the calling program. Thus only C run time functions
 * which assume that the DS != SS can be called in a DLL.
 * See 'Appendix B - C Run-time Functions' in the MS Windows SDK manual for
 * a list of functions which can or cannot be used in DLLs.
 ****************************************************************************/

#include <stdio.h>
#include "ca_prntf.h"

static int decimal, sign;

static int putbuf(int ch, char _FARARG_* _FARARG_*buf);

/****************************************************************************
 * CAsprintf
 ****************************************************************************/

int CAsprintf( char _FARARG_ *buf, char _FARARG_ *fmt, ... )
    {
    /*va_list args;*/
    char _FARARG_ *start = buf;
#ifdef WIN32
    va_list args;
#else
    char _FARARG_ *args;
#endif
 
    va_start( args, fmt );
    idoprnt( putbuf, &buf, fmt, args );
    *buf = '\0';
    return buf - start;
    }

/****************************************************************************
 * ca_printf
 ****************************************************************************/

int ca_printf( char _FARARG_ *fmt, ... )
    {
//    va_list _FARARG_ *args;
 
//    va_start( args, fmt );
//   idoprnt( fputc, stdout, fmt, args );
    return 0;
    }

/****************************************************************************
 * ca_fprintf
 ****************************************************************************/

int ca_fprintf( FILE _FARARG_ *stream, char _FARARG_ *fmt, ... )
    {
//    va_list args;
 
//    va_start( args, fmt );
//   idoprnt( fputc, stream, fmt, args );
    return 0;
    }

/****************************************************************************
 * IDOPRNT - workhorse function for printf/fprintf/sprintf.
 *
 *          (c) 1988 Allen I. Holub.  All rights reserved
 *
 *          Originally integer only but expanded at CA to support
 *          floating point formats %e and %f.
 *          This code copied from 4/88 Dr. Dobbs Journal of Software Tools.
 *
 *    The following conversions are supported:
 *
 *          %d %ld    decimal, long decimal
 *          %u        unsigned (int only, no longs)
 *          %s        string
 *          %x %lx    hex, long hex
 *          %o %lo    octal, long octal
 *          %b %lb    binary, long binary               (nonstandard)
 *          %p        far pointer (in hex xxxx:xxxx)
 *          %f %lf    float, double
 *          %e %le    float, double
 *          %E %lE    float, double
 *
 *          Note that it's impossible to get zero fill in the offset part of
 *          a %p conversion. That is 1234:0001 is always printed as 1234:1.
 *          You can use the %0p to pad out the segment portion, however.
 *
 *    The following modifiers are supported for all conversions:
 *
 *          %0x       zero left fill
 *          %-x       left justification in field
 *          %|x       centered in field                 (nonstandard)
 *          %*x       field width from first argument
 *
 *          Precision is supported for strings and floating point:
 *
 *          %X.Y      strings: X-character wide field, print at most Y
 *                             characters (even if the string is longer).
 *                             If X or Y is a *, the value is taken from
 *                             the next argument.
 *
 *                    floats:  X-character wide field, print Y digits after
 *                             the decimal point. Default precision=6.
 *
 *     
 *          Potential portability problems:
 *
 *          %p is a FAR pointer. I've used the "far" keyword to declare it
 *          as such and I've used the FP_SEG and FP_OFF identifiers to 
 *          extract the segment and offset parts of the pointer. The offset
 *          part of a near pointer can be printed by casting it to an int
 *          and using %x.
 *  
 ****************************************************************************/

#ifdef WIN32
void idoprnt( int (*out)(int, char _FARARG_* _FARARG_*),   // output function
              void _FARARG_ *o_param,             // 2nd arg to pass to out()
              char _FARARG_ *format,              // pointer to format string
              va_list        args )               // pointer to arguments
#else
void idoprnt( int (*out)(int, char _FARARG_* _FARARG_*),   // output function
              void _FARARG_ *o_param,             // 2nd arg to pass to out()
              char _FARARG_ *format,              // pointer to format string
              char _FARARG_ *args )               // pointer to arguments
#endif
    {
    char filchar;            //   fill char used to pad fields      
    char nbuf[34];           //   buffer used to hold converted #s  
    char _FARARG_ *bp;       //   pointer to current output buffer  
    int  slen;               //   len of string pointed to by bp    
    int  base;               //   current base (%x=16, %d=10, etc..)
    int  fldwth;             //   field width as in %10x            
    int  precision;          //   precision as in %10.3s or %10.4f  
    int  lftjust;            //   1= left justify (ie. %-10d)       
    int  center;             //   1= centered (ie. %|10d)           
    int  longf;              //   doing long int or float (ie. %lx) 
    int  fl_pnt;             //   doing floating point              
    int  neg_nbr;            //   neg number flag                   
    long lnum;               //   used to hold integer arguments    
    double dnum;             //   used to hold floating point args  
    void _FARARG_ *pnum=NULL;  //   pointer-sized number         

    for(; *format ; format++ )
      {
      if( *format != '%')          // if no conversion just print the char
        (*out)( *format, o_param );
      else                         // else process a % conversion
        {
        bp          = nbuf;       
        filchar     = ' ';
        fldwth      = 0;    
        lftjust     = 0;   
        center      = 0;    
        longf       = 0;     
        fl_pnt      = 0;     
        neg_nbr     = 0;     
        precision   = 0; 
        slen        = 0;      
        lnum        = 0L;
        dnum        = 0.0;

        // Interpret any modifiers that can precede a conversion char
        // (ie %04x or %-10.6s, etc...). If a * is present instead of a
        // field width then the width is taken from the argument list.

        if( *++format == '-')  { ++format ; ++lftjust;     }
        if( *format   == '|')  { ++format ; ++center;      }
        if( *format   == '0')  { ++format ; filchar = '0'; }

        if( *format != '*')  
          TOINT( format, fldwth );
        else
          { 
          ++format;
          fldwth = va_arg( args, int);
          }

        if( *format == '.')  
          if( *++format != '*')  
            TOINT( format, precision );
          else
            { 
            ++format;
            precision = va_arg( args, int);
            }

        if( *format == 'l' || *format == 'L')  
          { 
          ++format;
          ++longf;
          }

        // By now we've picked off all the modifiers and *format is
        // looking at the actual conversion character. Now pick the 
        // right size arg off the stack and advance the pointer (bp)
        // to point at the next arg.

        switch( *format )
          {
          default  :    *bp++ = *format;                break;
          case 'c' :    *bp++ = va_arg( args, int);     break;
          case 's' :    bp    = va_arg( args, char *);  break;

          case 'p' :
#ifdef WIN16
            pnum  = va_arg( args, void FAR *);
            bp    = ltos( (unsigned long)FP_SEG(pnum), bp, 16);
            *bp++ = ':';
            bp    = ltos( (unsigned long)FP_OFF(pnum), bp, 16);
#endif
            break;

          case 'x' :    base =  16 ;  goto pnum;
          case 'u' :    base = -10 ;  goto pnum;
          case 'd' :    base =  10 ;  goto pnum;
          case 'e' :    base =  10 ;  fl_pnt++;  goto pnum;
          case 'E' :    base =  10 ;  fl_pnt++;  goto pnum;
          case 'f' :    base =  10 ;  fl_pnt++;  goto pnum;
          case 'o' :    base =   8 ;  goto pnum;
          case 'b' :    base =   2 ;

pnum:       // Fetch a double, float, long, or int sized arg off the stack
            // as appropriate. If the fetched number is a base 10 int then
            // mask off the top bits to prevent sign extension.

            if ( fl_pnt )                 // floating point
              {
              dnum = (double) va_arg(args, double);
              if( precision == 0 )
                precision = 6;            // default precision for floats
              }
            else if ( longf )             // long int
              lnum = (long) va_arg(args, long);
            else                          // int
              {
              lnum = (long) va_arg(args, int);   
 
              if( base == -10 )           // unsigned int
                {
                base = 10;
                lnum &= INTMASK;
                }
              else if( base != 10 )       // nondecimal int
                lnum &= INTMASK;
              }
 
            if( lnum < 0L || dnum < 0.0)
              neg_nbr++;

            // For neg decimal nbrs print the - sign now 
            // to avoid the "000-123" situation.
            if( neg_nbr && base == 10 && filchar == '0' )
              {
              (*out)( '-', o_param );
              --fldwth;                 
              if( fl_pnt )
                dnum = -dnum;
              else
                lnum = -lnum;
              }
 
            if( fl_pnt )
              {
              if( *format == 'f' )  
                bp = ftos( dnum, bp, precision );
              else
                bp = etos( dnum, bp, precision, *format );
              }
            else
              bp = ltos( lnum, bp, base );
            break;
          }

        // Terminate the string if necessary and compute the string len
        // (slen). bp will point at the beginning of the output string.

        if( *format != 's')  
          {
          *bp  = '\0';
          slen = bp - (char _FARARG_ *)nbuf;
          bp   = nbuf;
          }
        else
          {
          slen = _fstrlen(bp);
          if( precision && slen > precision )              
            slen = precision;              
          }

        // Adjust fldwth to be the amount of padding we need to fill
        // the buffer out to the specified field width. Then print leading
        // padding (if we aren't left justifying), the buffer itself,
        // and any required trailing padding (if we are left justifying).

        if( (fldwth -= slen) < 0 )
          fldwth = 0;

        if( center )
          {
          longf = fldwth / 2;                     // use longf as counter
          PAD( longf, filchar, out, o_param );
          }
        else if( !lftjust )
          PAD( fldwth, filchar, out, o_param );

        while( --slen >= 0 )
          (*out)( *bp++, o_param );

        if( center )
          {
          longf = fldwth - fldwth / 2;
          PAD( longf, filchar, out, o_param );
          }
        else if( lftjust )
          PAD( fldwth, filchar, out, o_param );
        }
      }
    }

/****************************************************************************
 * ltos
 *
 * Purpose:
 *  Convert long to string. Prints in hex, decimal, octal, or binary.
 *
 *      "n" = the number to be converted
 *      "buf" = output buffer
 *      "base" = number base (16, 10, 8, or 2)
 *  
 *  The output string is null terminated and a pointer to the null
 *  terminator is returned.
 *
 *  The number is put into an array one digit at a time as it's
 *  translated. The array is filled with the digits reversed (ie. the
 *  \0 goes in first, the rightmost digit, etc..) and then is reversed
 *  in place before returning.
 *
 *  This routine is much like the unix() ltoa except that it returns
 *  a pointer to the end of the string.
 ****************************************************************************/

char _FARARG_ *ltos( unsigned long n, char _FARARG_ *buf, int base )
    {
    register char _FARARG_ *bp = buf;
    register int minus = 0;
    char _FARARG_ *endp;

    if( base < 2 || base > 16 ) 
      return 0;

    // If the nbr is neg and we're in base 10,
    // set minus to true and make it positive.
    if( base == 10 && (long)n < 0 )
      {
      minus++;                           
      n = -( (long)n );
      }

    // have to put null in now because the
    // array is being filled in reverse order
    *bp = '\0';                

    do {
       *++bp = "0123456789abcdef" [ n % base ];
       n /= base;
       }  while ( n );

    if( minus )
      *++bp = '-';

    // Reverse string in place using minus for temp storage.
    for( endp = bp; bp > buf; )
      {
      minus  = *bp;                   
      *bp -- = *buf;
      *buf++ = minus;
      }
  
    // return pointer to terminating null
    return endp;      
    }

/****************************************************************************
 * ftos
 *
 * Purpose:
 *  Convert floating point to %f format string.
 *
 *      "num" = the number to be converted
 *      "buf" = output buffer
 *      "pre" = nbr of places after decimal point
 *  
 *  The output string is null terminated and a pointer to the null
 *  terminator is returned.
 ****************************************************************************/

char _FARARG_ *ftos( double num, char _FARARG_ *buf, int pre )
    {
    register char _FARARG_ *bp = buf;
    char _FARARG_ *fp;
    register int j;
    /*int decimal, sign;*/
    int dec;

    buf[0] = '\0';

    // put in minus if neg
    if( num < 0.0 )
      {
      *bp++ = '-';
      *bp = '\0';
      }

    fp = _fcvt( num, pre, &decimal, &sign );

    if( decimal <= 0 )
      {
      dec = (abs(decimal) > pre) ? pre : abs(decimal);
      *bp++ = '0';                    // put in leading '0.'
      *bp++ = '.';
      for( j=0; j<dec; j++ )          // padd zeroes (ie 0.000...)
         *bp++ = '0';
      *bp = '\0';
      _fstrcat( buf, fp );            // add digits (ie 0.000123)
      }
    else
      {   
      _fstrcat( buf, fp );

      // now insert the dec point (fcvt just tells the position)
      for( j=pre; j>0; j-- )
        bp[decimal+j] = bp[decimal+j-1];
      bp[decimal] = '.';
      bp[decimal+pre+1] = '\0';
      }

    return( _fstrchr( buf, '\0' ) );
    }

/****************************************************************************
 * etos
 *
 * Purpose:
 *  Convert floating point to %e or %E format string.
 *
 *      "num" = the number to be converted
 *      "buf" = output buffer
 *      "pre" = nbr of places after decimal point
 *      "fmt" = e or E exponent character in output string
 *  
 *  The output string is null terminated and a pointer to the null
 *  terminator is returned.
 ****************************************************************************/

char _FARARG_ *etos( double num, char _FARARG_ *buf, int pre, char fmt )
    {
    register char _FARARG_ *bp = buf;
    char _FARARG_ *fp;
    char exp_str[6];
    /*int decimal, sign;*/
    int exp;

    buf[0] = '\0';

    if( num < 0.0 )
      *bp++ = '-';

    fp = _ecvt( num, pre+1, &decimal, &sign );

    // load nbr which goes before the decimal and the decimal point
    *bp++ = *fp++;
    *bp++ = '.';
    *bp = '\0';

    _fstrcat( buf, fp );

    bp = _fstrchr( buf, '\0' );
    *bp++ = fmt;                         // load exponent char e or E
    *bp++ = (decimal < 0) ? '-' : '+';   // load exponent sign
    *bp = '\0';

    exp = abs(decimal-1);                // always one less than dec value 

    if( exp < 100 )
      {
      *bp++ = '0';
      *bp = '\0';
      }
    if( exp < 10 )
      {
      *bp++ = '0';
      *bp = '\0';
      }

    ltos( (unsigned long)exp, exp_str, 10 );
    _fstrcat( buf, exp_str );
    return( _fstrchr( buf, '\0') );
    }

/****************************************************************************
 * putbuf - helper function for CAsprintf
 ****************************************************************************/

static int putbuf( int ch, char _FARARG_* _FARARG_*buf )
    {
    **buf = (char) ch;
    (*buf)++;
    return ch;
    }
