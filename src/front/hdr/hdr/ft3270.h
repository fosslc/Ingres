/*
**    Copyright (c) 2004 Ingres Corporation
*/

/**
**    Name:  FT3270.h  - IBM Specific Header File
**
**    Note that this header file lives with the CL Top Headers, rather
**    than within FT as you might expect. This is so that both the FT
**    and FSTM components will be able to share the same copy.
**    (FTibm.h is the same)
**
**    History:
**       10/01/85 scl  Cleanup
**/

/*
**                            3270 Orders
*/

/*
**    We need these special defines because sometimes UTS needs to refer
**    to the *real* orders instead of their ASCII versions.
*/
#define EBC_SBA 0x11
#define EBC_SF  0x1d
#define EBC_SA  0x28
#define EBC_SFE 0x29
#define EBC_IC  0x13
#define EBC_PT  0x05
#define EBC_GE  0x08
#define EBC_RA  0x3c

#ifdef EBCDIC

#define SBA 0x11
#define SF  0x1d
#define ESC 0x27
#define SA  0x28
#define SFE 0x29
#define IC  0x13
#define PT  0x05
#define GE  0x08
#define RA  0x3c

#else

#define SBA 0x11
#define SF  0x1d
#define ESC 0x1b
#define SA  0x1e
#define SFE 0x1f
#define IC  0x13
#define PT  0x09
#define GE  0x08
#define RA  0x14

#endif

#define BLNK 0xfe
#define ATR  0xff
/*
**
**             3270 Attention Identification Codes (AIDs)
**
*/
#define ENTER 0x7d
#define NOAID 0x60
#define PA1   0x6C
#define PA2   0x6E
#define PA3   0x6B
#define CLEAR 0x6D
#define PF1   0xf1
#define PF2   0xf2
#define PF3   0xf3
#define PF4   0xf4
#define PF5   0xf5
#define PF6   0xf6
#define PF7   0xf7
#define PF8   0xf8
#define PF9   0xf9
#define PF10  0x7a
#define PF11  0x7b
#define PF12  0x7c
#define PF13  0xc1
#define PF14  0xc2
#define PF15  0xc3
#define PF16  0xc4
#define PF17  0xc5
#define PF18  0xc6
#define PF19  0xc7
#define PF20  0xc8
#define PF21  0xc9
#define PF22  0x4a
#define PF23  0x4b
#define PF24  0x4c

/*
**                          Field Attributes
*/
#define HI        8
#define LOW       4
#define INVISIBLE 8+4
#define PROT      32
#define AUTOSKIP  16
#define UNPROT    0
#define NOFA      256

/*
**                     Extended Datastream Flags
*/
#define AT_FLD    0xc0
#define AT_XHI    0x41
#define AT_CLR    0x42
#define AT_PSS    0x43

/*
**                     Extended Highlighting Attributes
*/
#define BLINK      0xF1
#define REVERSE    0xF2
#define UNDERSCORE 0xF4

/*
**                           Color Attributes
*/
#define BLUE       0xF1
#define RED        0xF2
#define PINK       0xF3
#define GREEN      0xF4
#define TURQUOISE  0xF5
#define YELLOW     0xF6
#define WHITE      0xF7

/*
**                Write-Control-Character (WCC) Flags
*/
#define ALARM  4
#define UNLOCK 2
#define RESET  1

/*
**                 3270 Opcodes (Used by MVS only)
*/
#define WSF        0xf3
#define WRT        0xf1
#define ERSWRT     0xf5
#define ERSWRTA    0x7e
