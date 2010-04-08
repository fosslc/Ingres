#   Name: pc737.chs - IBM PC Code Page 737 
#
#   Description:
#
#       Standard PC code page for DOS Greek 
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
#       19-jul-2005 (rigka01)
#          Created. Bug 114739, problem INGNET173
#
0-8     c               # NUL,STX,SOT,ETX,EOT,ENQ,ACK,BEL,BS
9-D     cw		# HT, LF, VT, FF, CR
E-1F    c		# SO,SI,DLE,DC1,DC2,DC3,DC4,NAK,SYN,ETB,CAN,EM,SUB,ESC,FS,GS,RS,US
20      pw              # space
21-22   po              # !"
23-24   pon             # #$
25-2F   po              # %&'()*+,-./
30-39   pndx           	# 0-9 
3A-3F   po              # :;<=>?
40      pon             # @
41-46   psux    61      # A-F
47-5A   psu     67      # G-Z
5B-5E   po              # \]]^
5F      ps              # _ 
60      po              # ` 
61-66   pslx            # a-f
67-7A   psl             # g-z
7B-7E   po              # {|}~
7F      c		# DEL

80-91	pus	98	# greek capital letter alpha through sigma
92-96	pus	AB	# greek capital letter tau through psi
97	pus	E0	# greek capital omega 
98-A9	pls		# greek small letter alpha through sigma
AA	pas		# greek small letter final sigma 
AB-AF	pls		# greek small letter tau through psi
B0-B2	c		# light, medium, and dark shade
B3-DA	c		# box drawings
DB-DF	c		# blocks
E0	pls		# greek small letter omega
E1-E3	pls		# greek small letter alpha,epsilon,eta with tonos
E4	pls		# greek small letter iota with dialytika
E5-E7	pls		# greek small letter iota,omicron,upsilon with tonos
E8	pls		# greek small letter upsilon with dialytika
E9	pls		# greek small letter omega with tonos
EA-EC	pus	E1	# greek capital letter alpha,epsilon,eta with tonos	
ED-EF	pus	E5	# greek capital letter iota,omicron,upsilon with tonos	
F0	pus	E9	# greek capital letter omega with tonos
F1-F3	po		# plus-minus,greater than or equal, less than or equal
F4	pus	E4	# greek capital letter iota with dialytika 
F5	pus	E8	# greek capital letter upsilon with dialytika
F6-FB	po		# division sign, almost equal to, degree sign, bullet operator, middle dot, square root
FC-FD	c		# superscript latin small letter N, superscript two
FE-FF	c		# black square, no-break space
