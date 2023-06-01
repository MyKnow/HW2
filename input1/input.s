
input.o:     file format elf32-tradbigmips


Disassembly of section .text:

00000000 <main>:
   0:	27bdfff0 	addiu	sp,sp,-16
   4:	afbe000c 	sw	s8,12(sp)
   8:	03a0f025 	move	s8,sp
   c:	afc00000 	sw	zero,0(s8)
  10:	afc00004 	sw	zero,4(s8)
  14:	1000000a 	b	40 <main+0x40>
  18:	00000000 	nop
  1c:	8fc30000 	lw	v1,0(s8)
  20:	8fc20004 	lw	v0,4(s8)
  24:	00000000 	nop
  28:	00621021 	addu	v0,v1,v0
  2c:	afc20000 	sw	v0,0(s8)
  30:	8fc20004 	lw	v0,4(s8)
  34:	00000000 	nop
  38:	24420001 	addiu	v0,v0,1
  3c:	afc20004 	sw	v0,4(s8)
  40:	8fc20004 	lw	v0,4(s8)
  44:	00000000 	nop
  48:	2842000a 	slti	v0,v0,10
  4c:	1440fff3 	bnez	v0,1c <main+0x1c>
  50:	00000000 	nop
  54:	8fc20000 	lw	v0,0(s8)
  58:	03c0e825 	move	sp,s8
  5c:	8fbe000c 	lw	s8,12(sp)
  60:	27bd0010 	addiu	sp,sp,16
  64:	03e00008 	jr	ra
  68:	00000000 	nop
  6c:	00000000 	nop


addiu 	구현 완료
sw	구현 완료
move 	구현 완료
b	구현 완료
nop	구현 완료
lw	
slti	구현 완료
bnez	구현 완료
addu	구현 완료
jr



input.o:     file format elf32-tradbigmips


Disassembly of section .text:

00000000 <main>:
   0:	27bdffd8 	addiu	sp,sp,-40
   4:	afbf0024 	sw	ra,36(sp)
   8:	afbe0020 	sw	s8,32(sp)
   c:	03a0f025 	move	s8,sp
  10:	24020004 	li	v0,4
  14:	afc2001c 	sw	v0,28(s8)
  18:	8fc4001c 	lw	a0,28(s8)
  1c:	0c000000 	jal	0 <main>
  20:	00000000 	nop
  24:	03c0e825 	move	sp,s8
  28:	8fbf0024 	lw	ra,36(sp)
  2c:	8fbe0020 	lw	s8,32(sp)
  30:	27bd0028 	addiu	sp,sp,40
  34:	03e00008 	jr	ra
  38:	00000000 	nop

0000003c <foo>:
  3c:	27bdffe0 	addiu	sp,sp,-32
  40:	afbf001c 	sw	ra,28(sp)
  44:	afbe0018 	sw	s8,24(sp)
  48:	03a0f025 	move	s8,sp
  4c:	afc40020 	sw	a0,32(s8)
  50:	8fc30020 	lw	v1,32(s8)
  54:	24020001 	li	v0,1
  58:	14620004 	bne	v1,v0,6c <foo+0x30>
  5c:	00000000 	nop
  60:	24020001 	li	v0,1
  64:	1000000b 	b	94 <foo+0x58>
  68:	00000000 	nop
  6c:	8fc20020 	lw	v0,32(s8)
  70:	00000000 	nop
  74:	2442ffff 	addiu	v0,v0,-1
  78:	00402025 	move	a0,v0
  7c:	0c000000 	jal	0 <main>
  80:	00000000 	nop
  84:	00401825 	move	v1,v0
  88:	8fc20020 	lw	v0,32(s8)
  8c:	00000000 	nop
  90:	00621021 	addu	v0,v1,v0
  94:	03c0e825 	move	sp,s8
  98:	8fbf001c 	lw	ra,28(sp)
  9c:	8fbe0018 	lw	s8,24(sp)
  a0:	27bd0020 	addiu	sp,sp,32
  a4:	03e00008 	jr	ra
  a8:	00000000 	nop
  ac:	00000000 	nop










addiu 	구현 완료
sw	구현 완료
move 	구현 완료
b	구현 완료
nop	구현 완료
lw	구현 완료
slti	구현 완료
bnez	구현 완료
addu	구현 완료
jr	구현 완료
li	구현 완료..?
jal	구현 완료..?
bne
addu

