#   Name: ALT, russian PC-alternate 
#
#   Description:
#
#      Popular Russian Pc-DOS charset.
#		A bit like ISO 8859-1, with
#		enough stuff left out to make room
#		for russian alphabet
#
#   History:
#
#      27-dec-1995 (angusm) 
#           Rebuilt from old notes
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

80-8f	pus		a0		# Cyrillic uppercase ->PE
90-9f	pus		e0		# Cyrillic uppercase
a0-af	pls				# Cyrillic lowercase ->pe
b0-bf	po				# printable stuff
c0-cf	pas				# printable stuff
d0-d6	pus		f0		# printable stuff
d7	po				# multiplication
d8-de	pus		f8		# printable uppercase
df	po				# lowercase s-z
e0-ef	pls				# Cyrillic lowercase
f0-f6	pls				# printable lowercase
f7		po			# division
f8-fe	pls				# lowercase stuff
ff		po			# small y umlaut 
