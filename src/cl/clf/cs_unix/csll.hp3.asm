/*
**Copyright (c) 2004 Ingres Corporation
*/
    #	??-???-?? (?)
    #		Created.
    #	18-nov-90 (rog)
    #		Changed logic to use tas on those 300 series machines
    #		where CS_hp3_broken_tas is false, bset otherwise.  Also,
    #		changed most branches to be as short as possible and replaced
    #		some instructions with equivalent but quicker ones.

set CS__MACHINE_SPECIFIC,	0x20
set CS__CCSAVE,			0
set CS__PCSAVE,			0x4
set CS__REGSET,			0x8
set CS__CURRENT,		0xb8
set CS__INKERNEL,		0xcc
set CS_INITIALIZING,		0x10
set CS__SP,			CS__REGSET + (0xf * 4)

global	_CS_swuser, _CS_tas, _CS_getspin

_CS_swuser:
  # If server in kernel state, no task switching
	tst.l	_Cs_srv_block+CS__INKERNEL
	bne.b	tas_ret
  # if there is no current session, don't save registers
	mov.l	_Cs_srv_block+CS__CURRENT, %d0
	mov.l	%d0,%a0
	beq.b	CS__pick_new
  # at this point a0 points to the session control block
  # save the state
  # we keep the exception stack from EX in d0 in the register save array
	mov.l	_ex_sptr, %d0
	movm.l	&0xffff, (CS__MACHINE_SPECIFIC+CS__REGSET)(%a0)
CS__pick_new:
  # run xchng thread on idle thread's stack
	mov.l	_Cs_idle_scb+CS__MACHINE_SPECIFIC+CS__SP, %sp
  # and use idle thread's exception stack as well
	mov.l	_Cs_idle_scb+CS__MACHINE_SPECIFIC+CS__REGSET, _ex_sptr
  # new_scb = CS_xchng_thread(old_scb)
	pea.l	(%a0)
	jsr	_CS_xchng_thread
  # don't bother to pop arguement off stack since we change the sp anyway
  # at return time, d0 has new current SCB, move it and go
	mov.l	%d0, %a0
	movm.l	(CS__MACHINE_SPECIFIC+CS__REGSET)(%a0), &0xffff
  # recover the exception stack
	mov.l	%d0,_ex_sptr
	rts

_CS_tas:
	mov.l	4(%sp), %a0	# get the argument value into %a0
	movq.l	&0, %d0		# set return value to zero (quicker than clr)
	tst.b	_CS_hp3_broken_tas	# can we use tas or bset?
	bne.b	broken_tas	# must use bset
	tas.b	(%a0)		# do test a bit and set instruction
	bne.b	tas_ret		# bit tested was non-zero (failed)
passed:
	movq.l	&1,%d0		# set return value to one (passed)
tas_ret:
	rts			# return
broken_tas:
	bset.b	&1,(%a0)	# do test a bit and set instruction
	beq.b	passed		# bit tested was zero (passed)
	br.b	tas_ret		# bit tested was non-zero (failed)

_CS_getspin:
	mov.l	4(%sp), %a0	# get argument into %a0
	tst.b   _CS_hp3_broken_tas # can we use tas or bset?
	bne.b   bad_tas		# must use bset
	tas.b	(%a0)		# do test a bit and set instruction
	beq.b	tas_ret		# success, we have lock, return
csspinit:
	tas.b	(%a0)		# do test a bit and set again
	bne.b	csspinit	# failed, spin until we get lock
collision:
	add.w	&1, 2(%a0)	# got lock, bump collision counter
	rts			# return
bad_tas:
	bset.b	&1,(%a0)	# do test a bit and set instruction
	beq.b	tas_ret		# success, we have a lock, return
badspinit:
	bset.b	&1,(%a0)	# do test a bit and set instruction
	bne.b	badspinit	# failed, spin until we get a lock
	br.b	collision	# got lock, go exit
