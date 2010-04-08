#   Name: IBM code page 866, DOS-Cyrillic 
#
#   Description:
#
#      Popular Russian Pc-DOS charset.
#
#   History:
#
#      20-jun-1996 (angusm)
#			initial version
#
0-8     c               # Control characters
9-D     cw				# white-space
E-14    c				# control
15      p               # section character
16-1F   c				# control
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
7F      c				#

80-8f	pus		a0		# Cyrillic uppercase ->PE
90-9f	pus		e0		# Cyrillic uppercase
a0-af	pls				# Cyrillic lowercase ->pe

b0-df   c               # graphics (line-drawing) characters

e0-ef	pls				# Cyrillic lowercase
f0		pus		f1		# Uppercase IO
f1		pls				# lowercase IO
f2		pus		f3		# Uppercase Ukrainian IE
f3		pls				# Lowercase Ukrainian IE
f4		pus		f5		# Uppercase YI
f5		pls				# lowercase YI
f6		pus		f7		# uppercase short U
f7		pls				# lowercase short u
f8-fd	po				# various printables
fe-ff	c				# control 
