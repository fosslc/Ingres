#   Name: iso88597.chs - ISO Coded Character Set 8859/7
#
#   Description:
#
#       Character classifications for ISO Coded Character Set 8859/7
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
#	01-jul-2005 (rigka01)
#	   Created. Bug 114739, problem INGNET173 
#	   20 through 7F copied from is885915.chs due to
#	   matching characters in this range. 
#
0-8	c		# ISO 8859-7 
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
7F-9F	c		# undefined
A0	cw		# no-break space
A1-A3	po		# reversed comma, apostrophe, pound sign 
A4-A5	c		# undefined
A6-A9	po		# broken bar, section sign, diaeresis, copyright sign
AA	c		# undefined
AB-AD	po		# left pointing double angle quotation, not sign, soft hyphen
AE	c		# undefined
AF-B5	po              # horizontal bar, degree sign, plus-minus sign, superscript two, superscript three, greek tonos, greek dialytika tonos
B6	pus	DC	# greek capital letter alpha with tonos
B7	po		# middle dot
B8-BA   pus	DD	# greek capital letters epsilon,eta,iota with tonos
BB	po		# right-pointing double angle quotation mark
BC	pus	FC	# greek capital letter omicron with tonos
BD	po		# vulgar fraction one half
BE-BF	pus	FD	# greek capital letter upsilon,omega with tonos
C0	pas		# greek small letter iota with dialytika and tonos 
C1-D1	pus	E1	# greek capital letters alpha through rho
D2	c		# undefined
D3-DB	pus	F3	# greek capital letters sigma through omega, greek captial letters iota,upsilon with dialytika
DC-DF	pls		# greek small letters alpha,epsilon,eta,iota with tonos
E0	pas		# greek small letter upsilon with dialytika and tonos 
E1-F1	pls		# greek small letters alpha through rho
F2	pas		# greek small letter final sigma
F3-FB	pls		# greek small letters sigma through omega, greek small letters iota,upsilon with dialytika
FC	pls		# greek small letter omicron with tonos
FD-FE	pls		# greek small letters upsilon,omega with tonos
FF	c		# undefined
