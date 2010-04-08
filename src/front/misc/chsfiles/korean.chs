#  Name: korean
#
#  History:
#	29-may-1998 (canor01, for yosts50)
#	    According to Fujitsu Korea people, they need to support 
#	    extend korean characters also as follows:
#	       - The code between 0x81 to 0xa0 can be the first byte 
#	         of double byte.
#	       - The code between 0x41 to 0xa0 except for 0x5b-0x5f, 0x60 and
#                0x7b-0x7f can be the second byte of double byte.
#
0-8	c	# korean
9-D     cw      # CR's, LF's, FF's, etc.
20      pw      # space
21-22   po      # !"
23-24   pon     # #$
25-2F   po      # various printable operators
30-39   pndx    # digits
3A-3F   po      # various printable operators
40      pon     # "at" sign

41-46   p2sux    61      # A-F
47-5A   p2su     67      # G-Z

5B-5E   po      # various printable operators
5F      ps      # underscore
60      po     # grave
61-66   p2slx   # a-f
67-7A   p2sl    # g-z
7B-7E   po      # various printable operators

81-A0	12sna	#
A1-FE	12sna	# printable 1st and 2nd byte character
