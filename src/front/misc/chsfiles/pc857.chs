#   Name: pc857.chs - IBM PC Code Page 857 
#
#   Description:
#
#       Standard PC code page for Turkish 
#
#   History:
#
#       Feb-1996 (angusm) 
#           Recreated.
#
0-8     c               # Control characters
9-D     cw
E-14    c
15      p               # section character
16-1F   c
20      pw              # space
21-22   po              # !"
23-24   pon             # #$
25-2F   po              # %&'()*+,-./
30-39   pndx            # decimal digits
3A-3F   po              # :;<=>?
40      pon             # "at" sign
41-46   psux    61      # A-F
47-5A   psu     67      # G-Z
5B-5E   po              # \]]^
5F      ps              # underscore
60      po              # apostrophe
61-66   pslx            # a-f
67-7A   psl             # g-z
7B-7E   po              # {|}~
7F      c

80		psu		87		# C cedilla
81-8C	psl				# various accented lowercase
8D		psa				# i-nodot: lower, no upper
8E		psu		84		# A diaresis
8F		psu		86		# A ring above
90		psu		82		# E acute
91		psl				# small ligature ae 
92		psu		91		# capital ligature ae 
93-97	psl				# various accented lowercase
98		psa				# I dotabove (appears to have no lowercase)
99		psu		94		# O diaresis
9A		psu		81		# U diaresis
9B		psl				# o w stroke
9C		po				# pound sign
9D		psu		9B		# O w stroke
9E		psu		9F		# S cedilla
9F		psl				# s cedilla
A0-A4	psl				# various
A5		psu		A4		# N tilde
A6		psu		A7		# G breve 
A7		psl				# g breve
A8-A9	po				#
AA-AF	po				#
B0-B4	c				# graphics
B5		psu		a0		# A acute 
B6		psu		83		# A circumflex
B7		psu		85		# A grave 
B8		po				# copyright
B9-BC	c				#
BD		po				# cent
BE		po				# yen 
BF		c				#
C0-C5	c				#
C6		psl				# a tilde 
C7		psu		c6		# A tilde 
C8-CE	c				#
CF		po				# that currency symbol
D0-D1	po				#
D2		psu		88		# E circumflex	
D3		psu	    89		# E diaresis
D4		psu		8a		# E grave
D5		c				#
D6		psu	    a1		# I acute 
D7		psu	    8c		# I circumflex 
D8		psu	    8b		# I diaresis
D9-DD	c				#
DE		psu		ec		# I grave
DF		c				#
E0		psu		a2		# O acute
E1		psa				# sharp s: lower only, no upper
E2		psu		93		# O circumflex
E3		psu	    95		# O grave
E4		psl				# o tilde
E5		psu		e4		# O tilde
E6		po				# micro sign
E7		c				#
E8		po				# multiplication sign
E9		psu		a3		# U acute
EA		psu		96		# U circumflex
EB		psu		97		# U grave	
EC		psl				# i grave	
ED		psa				# y diaresis: lower only, no uppper
EE-EF	po				# 
F0-F1	po				#
F2		c				#
F3-FD	po				# 
FE		c				#
ff		pw				# no-break space
