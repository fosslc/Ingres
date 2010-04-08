# CW.chs
#
#   Character set CW - Cyrillic for Windows 
#		CP 1251
#
#   History:
#	11-dec-1995 (angusm)
#		reassembled from old notes
#		broadly: for 7-bit 
#					same as ASCII
#			     for 8-bit
#					up to C0, misc slavonic chars : 
#					above C0, russian alphabet.
#
0-8     c				# control
9-D     cw              # whitespace 
e-1f	c				# control
20      pw              # blank 
21-22   po              #
23-24   pon             #
25-2F   po              #
30-39   pndx            # digits
3A-3F   po              #
40      pon             #

41-46   psux    61      #
47-5A   psu     67      #

5B-5E   po              #
5F      ps              #
60      po              #
61-66   pslx            #
67-7A   psl             #
7B-7E   po              #
7F      c				# control

80     	pus     90		# Cyrillic capital DJE 
81     	pus		83		# Cyrillic capital GJE 
82     	po				# punctuation 
83     	pls				# Cyrillic lowercase GJE 
84-85   po				# punctuation 
86-87	po				# printable opearator      
88		c				# not used
89      po				# punctuation
8A     	pus		9a		# Cyrillic capital LJE 
8B     	po				# punctuation 
8C     	pus		9c		# Cyrillic capital NJE 
8D     	pus		9d		# Cyrillic capital KJE
8E     	pus		9e		# Cyrillic capital TSHE
8F     	pus		9f		# Cyrillic capital DZHE

90     	pls				# Cyrillic small DJE 
91-97	po				# punctuation      
98     	c				# ? 
99      po				# punctuation
9A     	pls				# Cyrillic lowercase LJE 
9B     	po				# punctuation  
9C     	pls				# Cyrillic lowercase NJE
9D     	pls				# Cyrillic lowercase KJE
9E     	pls				# Cyrillic lowercase TSHE
9F     	pls				# Cyrillic lowercase DZHE


A0     	cw				# non-breaking space
A1     	pus		a2		# Cyrillic capital short U
A2     	pls				# Cyrillic lowercase short u 
A3     	pus	    bc		# Cyrillic capital JE
A4     	po				# Currency sign
A5     	pus		b4		# Cyrillic capital GHE w upturn
A6-A7	po				# punctuation
a8		pus		b8		# E diaresis (capital IO)
a9		po				# punctuation
aa		pus		ba		# Cyrillic capital ukrainian IE
ab		po				# punctuation
ac-ad	po				# printable things
ae		po				# punctuation
af		pus		bf		# Cyrillic capital YI

B0		po				# punctuation 
b1		po				# po
b2		pus		b3		# Cyrillic capital byelorussianb/ukrainian I
b3		pls				# Cyrillic lowercase byelorussianb/ukrainian I
b4		pls				# Cyrillic lowercase GHE w upturn
B5     	po				# micro 
B6-B7  	po				# punctuation 
B8     	pls				# e diaresis (lowercase io)
B9		po				# punctuation
ba		pls				# Cyrillic lowercase ukrainian IE 
bb		po				# punctuation
BC  	pls				# Cyrillic lowercase JE
BD     	pus		be		# Cyrillic capital DZE
BE     	pls				# Cyrillic lowercase  DZE
BF     	pls				# Cyrillic lowercase YI 

C0-DF	pus	E0			# Russian alphabet uppercase
E0-FF	pls				# Russian alphabet lowercase
