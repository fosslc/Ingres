#   Name: KOI-8 (ISO 6937/8) 
#
#   Description:
#
#      Popular Russian charset.
#
#   History:
#
#      27-dec-1995 (angusm) 
#           Initial version
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
80-9f	c				# box-drawing stuff etc
a0-a1	c				# stuff
a2		pls				# lowercase cyrillic io (e with umlaut/diaresis) 
a3-b1	c				# stuff
b2		pus		a2		# uppercase cyrillic io (E with umlaut/diaresis)
b3-bf	c				# stuff
c0-df	pls				# lowercase russian letters
e0-ff	pus		c0		# uppercase russian letters
