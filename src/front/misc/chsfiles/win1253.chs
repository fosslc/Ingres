#   Name: win1253.chs - Windows Code Page 1253
#
#   Description:
#
#       Character classifications for Windows Code Page 1253
#       Character Set Classification Codes:
#        p - printable
#        c - control
#        o - operator
#        x - hex digit
#        d - digit
#        u - upper-case alpha
#        l - lower-case alpha
#        a - non-cased alpha
#        w - whitespace
#        s - name start
#        n - name
#        1 - first byte of double-byte character
#        2 - second byte of double-byte character
#
#   History:
#       04-jul-2005 (rigka01)
#          Created. Bug 114739, problem INGNET173
#
0-8	c		# NUL,STX,SOT,ETX,EOT,ENQ,ACK,BEL,BS
9-D	cw		# HT, LF, VT, FF, CR
E-1F	c		# SO,SI,DLE,DC1,DC2,DC3,DC4,NAK,SYN,ETB,CAN,EM,SUB,ESC,FS,GS,RS,US
20	pw		# space
21-22	po		# !"
23-24	pon		# #$
25-2F	po		# %&'()*+,-./
30-39	pdxn		# 0-9
3A-3F	po		# :;<=>?
40	pon		# @
41-46	pxus	61	# A-F
47-5A	pus	67	# G-Z
5B-5E	po		# [\]^
5F	ps		# _
60	po		# `
61-66	pxls		# a-f
67-7A	pls		# g-z
7B-7E	po		# {|}~
7F	c		# DEL

80	po		# Euro sign
81	c		# undefined
82	po		# single low-9 quotation  
83	psa		# small letter f with hook 
84	po		# double low-9 quotation  
85-87 	po		# horizontal ellipsis, dagger, double dagger	
88	c		# undefined
89	po		# per mille sign 
8A	c		# undefined
8B	po		# single left pointing angle quotation mark 
8C-90	c		# undefined
91-97	po		# quotations, bullet, dashes 
98	c		# undefined
99	po		# trad mark sign 
9A	c		# undefined
9B	po		# single right pointing angle quotation mark 
9C-9F	c		# undefined

A0	cw		# no-break space
A1	po		# dialytika tonos
A2	pus	DC	# greek capital letter alpha with tonos 
A3-A9	po		# pound sign, currency sign,yen sign, broken bar, section sign, diaersis,  copyright sign
AA	c		# undefined
AB-AF	po		# left pointing double angle quotation, not sign, soft hyphen,registered sign, horizontal bar 
B0-B7   po              # degree, plus-minus,superscript 2, superscript 3,greek tonos, micro sign, pilcrow sign, middle dot
B8-BA	pus	DD	# Greek capital epsilon,eta,iota with tonos
BB      po              # right-pointing double angle quotation mark
BC      pus     FC      # greek capital letter omicron with tonos
BD      po              # vulgar fraction one half
BE-BF   pus     FD      # greek capital letter upsilon,omega with tonos
C0      pas             # greek small letter iota with dialytika and tonos
C1-D1   pus     E1      # greek capital letters alpha through rho
D3-D9   pus     F3      # greek capital letters sigma through omega
DA-DB   pus     FA      # greek capital letters iota,upsilon with dialytika
DC      pls             # greek small letter alpha with tonos
DD-DF   pls             # greek small letters epsilon,eta,iota with tonos
E0      pas             # greek small letter upsilon with dialytika and tonos
E1-F1   pls             # greek small letters alpha through rho
F2      pas             # greek small letter final sigma
F3-F9   pls             # greek small letters sigma through omega
FA-FB   pls             # greek small letters iota,upsilon with dialytika
FC      pls             # greek small letter omicron with tonos
FD-FE   pls             # greek small letters upsilon,omega with tonos
FF	c		# undefined	
