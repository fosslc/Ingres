$ !				INGRES_STARTUP.COM
$ goto beginning

	This file contains the definition(s) of the II_SYSTEM
	logical name, and call(s) to the startup command procedure(s).

	Multiple installations may exist on a single system, so they
	must be kept seperate. Please ensure that the correct
	definitions and calls are made for your environment.

$ beginning:
