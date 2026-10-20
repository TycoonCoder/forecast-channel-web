/* 0x800ea970  size=36  callers=922  strings=[] */
0x800ea970:  2c030000  cmpwi	r3, 0
0x800ea974:  7c8000a6  mfmsr	r4
0x800ea978:  4182000c  beq	0x800ea984
0x800ea97c:  60858000  ori	r5, r4, 0x8000
0x800ea980:  48000008  b	0x800ea988
0x800ea984:  5485045e  rlwinm	r5, r4, 0, 0x11, 0xf
0x800ea988:  7ca00124  mtmsr	r5
0x800ea98c:  54838ffe  rlwinm	r3, r4, 0x11, 0x1f, 0x1f
0x800ea990:  4e800020  blr	

/* 0x800ea948  size=20  callers=751  strings=[] */
0x800ea948:  7c6000a6  mfmsr	r3
0x800ea94c:  5464045e  rlwinm	r4, r3, 0, 0x11, 0xf
0x800ea950:  7c800124  mtmsr	r4
0x800ea954:  54638ffe  rlwinm	r3, r3, 0x11, 0x1f, 0x1f
0x800ea958:  4e800020  blr	

/* 0x80004338  size=44  callers=366  strings=[] */
0x80004338:  7c041840  cmplw	r4, r3
0x8000433c:  41800028  blt	0x80004364
0x80004340:  3884ffff  addi	r4, r4, -1
0x80004344:  38c3ffff  addi	r6, r3, -1
0x80004348:  38a50001  addi	r5, r5, 1
0x8000434c:  4800000c  b	0x80004358
0x80004350:  8c040001  lbzu	r0, 1(r4)
0x80004354:  9c060001  stbu	r0, 1(r6)
0x80004358:  34a5ffff  addic.	r5, r5, -1
0x8000435c:  4082fff4  bne	0x80004350
0x80004360:  4e800020  blr	

/* 0x800e675c  size=4  callers=313  strings=[] */
0x800e675c:  9421ff80  stwu	r1, -0x80(r1)

/* 0x801619c4  size=24  callers=287  strings=[] */
0x801619c4:  936bffec  stw	r27, -0x14(r11)
0x801619c8:  938bfff0  stw	r28, -0x10(r11)
0x801619cc:  93abfff4  stw	r29, -0xc(r11)
0x801619d0:  93cbfff8  stw	r30, -8(r11)
0x801619d4:  93ebfffc  stw	r31, -4(r11)
0x801619d8:  4e800020  blr	

/* 0x80161a10  size=24  callers=287  strings=[] */
0x80161a10:  836bffec  lwz	r27, -0x14(r11)
0x80161a14:  838bfff0  lwz	r28, -0x10(r11)
0x80161a18:  83abfff4  lwz	r29, -0xc(r11)
0x80161a1c:  83cbfff8  lwz	r30, -8(r11)
0x80161a20:  83ebfffc  lwz	r31, -4(r11)
0x80161a24:  4e800020  blr	

/* 0x8000443c  size=4  callers=266  strings=[] */
0x8000443c:  9421fff0  stwu	r1, -0x10(r1)

/* 0x8016184c  size=40  callers=191  strings=[] */
0x8016184c:  800c0000  lwz	r0, 0(r12)
0x80161850:  816c0004  lwz	r11, 4(r12)
0x80161854:  818c0008  lwz	r12, 8(r12)
0x80161858:  7c630214  add	r3, r3, r0
0x8016185c:  2c0b0000  cmpwi	r11, 0
0x80161860:  4180000c  blt	0x8016186c
0x80161864:  7d83602e  lwzx	r12, r3, r12
0x80161868:  7d8c582e  lwzx	r12, r12, r11
0x8016186c:  7d8903a6  mtctr	r12
0x80161870:  4e800420  bctr	

/* 0x801360f0  size=24  callers=184  strings=[] */
0x801360f0:  880da348  lbz	r0, -0x5cb8(r13)
0x801360f4:  2c000000  cmpwi	r0, 0
0x801360f8:  4c820020  bnelr	
0x801360fc:  4cc63182  crclr	cr1eq
0x80136100:  4bffff1c  b	0x8013601c
0x80136104:  4e800020  blr	

/* 0x801617e0  size=48  callers=168  strings=[] */
0x801617e0:  80a30000  lwz	r5, 0(r3)
0x801617e4:  80c30004  lwz	r6, 4(r3)
0x801617e8:  80e30008  lwz	r7, 8(r3)
0x801617ec:  38600001  li	r3, 1
0x801617f0:  2c050000  cmpwi	r5, 0
0x801617f4:  2f060000  cmpwi	cr6, r6, 0
0x801617f8:  2f870000  cmpwi	cr7, r7, 0
0x801617fc:  4c820020  bnelr	
0x80161800:  4c9a0020  bnelr	cr6
0x80161804:  4c9e0020  bnelr	cr7
0x80161808:  38600000  li	r3, 0
0x8016180c:  4e800020  blr	

/* 0x800325f8  size=12  callers=163  strings=[] */
0x800325f8:  7c641b78  mr	r4, r3
0x800325fc:  806d9590  lwz	r3, -0x6a70(r13)
0x80032600:  480de338  b	0x80110938

/* 0x801619c0  size=4  callers=162  strings=[] */
0x801619c0:  934bffe8  stw	r26, -0x18(r11)

/* 0x80161a0c  size=4  callers=162  strings=[] */
0x80161a0c:  834bffe8  lwz	r26, -0x18(r11)

/* 0x801360d8  size=24  callers=158  strings=[] */
0x801360d8:  880da348  lbz	r0, -0x5cb8(r13)
0x801360dc:  2c000000  cmpwi	r0, 0
0x801360e0:  4c820020  bnelr	
0x801360e4:  4cc63182  crclr	cr1eq
0x801360e8:  4bffff34  b	0x8013601c
0x801360ec:  4e800020  blr	

/* 0x800ebc10  size=4  callers=148  strings=[] */
0x800ebc10:  9421ffe0  stwu	r1, -0x20(r1)

/* 0x8016f588  size=28  callers=124  strings=[] */
0x8016f588:  3884fffe  addi	r4, r4, -2
0x8016f58c:  38a3fffe  addi	r5, r3, -2
0x8016f590:  a4040002  lhzu	r0, 2(r4)
0x8016f594:  2c000000  cmpwi	r0, 0
0x8016f598:  b4050002  sthu	r0, 2(r5)
0x8016f59c:  4082fff4  bne	0x8016f590
0x8016f5a0:  4e800020  blr	

/* 0x800ebb34  size=4  callers=123  strings=[] */
0x800ebb34:  9421ffe0  stwu	r1, -0x20(r1)

/* 0x801619bc  size=4  callers=123  strings=[] */
0x801619bc:  932bffe4  stw	r25, -0x1c(r11)

/* 0x80161a08  size=4  callers=123  strings=[] */
0x80161a08:  832bffe4  lwz	r25, -0x1c(r11)

/* 0x800362d8  size=4  callers=116  strings=[] */
0x800362d8:  9421ffc0  stwu	r1, -0x40(r1)

/* 0x80133028  size=80  callers=115  strings=[] */
0x80133028:  9421ff90  stwu	r1, -0x70(r1)
0x8013302c:  40860024  bne	cr1, 0x80133050
0x80133030:  d8210028  stfd	f1, 0x28(r1)
0x80133034:  d8410030  stfd	f2, 0x30(r1)
0x80133038:  d8610038  stfd	f3, 0x38(r1)
0x8013303c:  d8810040  stfd	f4, 0x40(r1)
0x80133040:  d8a10048  stfd	f5, 0x48(r1)
0x80133044:  d8c10050  stfd	f6, 0x50(r1)
0x80133048:  d8e10058  stfd	f7, 0x58(r1)
0x8013304c:  d9010060  stfd	f8, 0x60(r1)
0x80133050:  90610008  stw	r3, 8(r1)
0x80133054:  9081000c  stw	r4, 0xc(r1)
0x80133058:  90a10010  stw	r5, 0x10(r1)
0x8013305c:  90c10014  stw	r6, 0x14(r1)
0x80133060:  90e10018  stw	r7, 0x18(r1)
0x80133064:  9101001c  stw	r8, 0x1c(r1)
0x80133068:  91210020  stw	r9, 0x20(r1)
0x8013306c:  91410024  stw	r10, 0x24(r1)
0x80133070:  38210070  addi	r1, r1, 0x70
0x80133074:  4e800020  blr	

/* 0x801337e0  size=4  callers=115  strings=[] */
0x801337e0:  9421fff0  stwu	r1, -0x10(r1)

/* 0x8016f5a4  size=44  callers=107  strings=[] */
0x8016f5a4:  3884fffe  addi	r4, r4, -2
0x8016f5a8:  38a3fffe  addi	r5, r3, -2
0x8016f5ac:  a4050002  lhzu	r0, 2(r5)
0x8016f5b0:  2c000000  cmpwi	r0, 0
0x8016f5b4:  4082fff8  bne	0x8016f5ac
0x8016f5b8:  38a5fffe  addi	r5, r5, -2
0x8016f5bc:  a4040002  lhzu	r0, 2(r4)
0x8016f5c0:  2c000000  cmpwi	r0, 0
0x8016f5c4:  b4050002  sthu	r0, 2(r5)
0x8016f5c8:  4082fff4  bne	0x8016f5bc
0x8016f5cc:  4e800020  blr	

/* 0x801613d0  size=24  callers=105  strings=[] */
0x801613d0:  800da378  lwz	r0, -0x5c88(r13)
0x801613d4:  90050000  stw	r0, 0(r5)
0x801613d8:  90850004  stw	r4, 4(r5)
0x801613dc:  90650008  stw	r3, 8(r5)
0x801613e0:  90ada378  stw	r5, -0x5c88(r13)
0x801613e4:  4e800020  blr	

/* 0x801336f8  size=4  callers=101  strings=[] */
0x801336f8:  9421fff0  stwu	r1, -0x10(r1)

/* 0x80167b9c  size=4  callers=97  strings=[] */
0x80167b9c:  9421ff60  stwu	r1, -0xa0(r1)

/* 0x800f8740  size=204  callers=96  strings=[] */
0x800f8740:  9421ffc0  stwu	r1, -0x40(r1)
0x800f8744:  e0030000  .byte	0xe0, 0x03, 0x00, 0x00
0x800f8748:  d9c10008  stfd	f14, 8(r1)
0x800f874c:  e0c40000  .byte	0xe0, 0xc4, 0x00, 0x00
0x800f8750:  3cc08033  lis	r6, -0x7fcd
0x800f8754:  e0e40008  .byte	0xe0, 0xe4, 0x00, 0x08
0x800f8758:  d9e10010  stfd	f15, 0x10(r1)
0x800f875c:  38c605c0  addi	r6, r6, 0x5c0
0x800f8760:  dbe10028  stfd	f31, 0x28(r1)
0x800f8764:  e1040010  .byte	0xe1, 0x04, 0x00, 0x10
0x800f8768:  11860018  .byte	0x11, 0x86, 0x00, 0x18
0x800f876c:  e0430010  .byte	0xe0, 0x43, 0x00, 0x10
0x800f8770:  11a70018  .byte	0x11, 0xa7, 0x00, 0x18
0x800f8774:  e3e60000  .byte	0xe3, 0xe6, 0x00, 0x00
0x800f8778:  11c60098  .byte	0x11, 0xc6, 0x00, 0x98
0x800f877c:  e1240018  .byte	0xe1, 0x24, 0x00, 0x18
0x800f8780:  11e70098  .byte	0x11, 0xe7, 0x00, 0x98
0x800f8784:  e0230008  .byte	0xe0, 0x23, 0x00, 0x08
0x800f8788:  1188601e  .byte	0x11, 0x88, 0x60, 0x1e
0x800f878c:  e0630018  .byte	0xe0, 0x63, 0x00, 0x18
0x800f8790:  11c8709e  .byte	0x11, 0xc8, 0x70, 0x9e
0x800f8794:  e1440020  .byte	0xe1, 0x44, 0x00, 0x20
0x800f8798:  11a9681e  .byte	0x11, 0xa9, 0x68, 0x1e
0x800f879c:  e1640028  .byte	0xe1, 0x64, 0x00, 0x28
0x800f87a0:  11e9789e  .byte	0x11, 0xe9, 0x78, 0x9e
0x800f87a4:  e0830020  .byte	0xe0, 0x83, 0x00, 0x20
0x800f87a8:  e0a30028  .byte	0xe0, 0xa3, 0x00, 0x28
0x800f87ac:  118a605c  .byte	0x11, 0x8a, 0x60, 0x5c
0x800f87b0:  11ab685c  .byte	0x11, 0xab, 0x68, 0x5c
0x800f87b4:  11ca70dc  .byte	0x11, 0xca, 0x70, 0xdc
0x800f87b8:  11eb78dc  .byte	0x11, 0xeb, 0x78, 0xdc
0x800f87bc:  f1850000  xsaddsp	f12, f5, f0
0x800f87c0:  10460118  .byte	0x10, 0x46, 0x01, 0x18
0x800f87c4:  11bf685e  .byte	0x11, 0xbf, 0x68, 0x5e
0x800f87c8:  10070118  .byte	0x10, 0x07, 0x01, 0x18
0x800f87cc:  f1c50010  xxsldwi	vs14, vs5, vs0, 0
0x800f87d0:  11ff78de  .byte	0x11, 0xff, 0x78, 0xde
0x800f87d4:  f1a50008  xsmaddasp	f13, f5, f0
0x800f87d8:  1048111e  .byte	0x10, 0x48, 0x11, 0x1e
0x800f87dc:  1009011e  .byte	0x10, 0x09, 0x01, 0x1e
0x800f87e0:  104a115c  .byte	0x10, 0x4a, 0x11, 0x5c
0x800f87e4:  c9c10008  lfd	f14, 8(r1)
0x800f87e8:  f1e50018  xscmpeqdp	vs15, f5, f0
0x800f87ec:  100b015c  .byte	0x10, 0x0b, 0x01, 0x5c
0x800f87f0:  f0450020  .byte	0xf0, 0x45, 0x00, 0x20
0x800f87f4:  101f015e  .byte	0x10, 0x1f, 0x01, 0x5e
0x800f87f8:  c9e10010  lfd	f15, 0x10(r1)
0x800f87fc:  f0050028  .byte	0xf0, 0x05, 0x00, 0x28
0x800f8800:  cbe10028  lfd	f31, 0x28(r1)
0x800f8804:  38210040  addi	r1, r1, 0x40
0x800f8808:  4e800020  blr	

/* 0x80165354  size=60  callers=95  strings=[] */
0x80165354:  38e4ffff  addi	r7, r4, -1
0x80165358:  38c3ffff  addi	r6, r3, -1
0x8016535c:  38850001  addi	r4, r5, 1
0x80165360:  48000030  b	0x80165390
0x80165364:  8c660001  lbzu	r3, 1(r6)
0x80165368:  8c070001  lbzu	r0, 1(r7)
0x8016536c:  7c030040  cmplw	r3, r0
0x80165370:  41820020  beq	0x80165390
0x80165374:  88860000  lbz	r4, 0(r6)
0x80165378:  38600001  li	r3, 1
0x8016537c:  88070000  lbz	r0, 0(r7)
0x80165380:  7c040040  cmplw	r4, r0
0x80165384:  4c800020  bgelr	
0x80165388:  3860ffff  li	r3, -1
0x8016538c:  4e800020  blr	

/* 0x8007b9cc  size=32  callers=87  strings=[] */
0x8007b9cc:  80030018  lwz	r0, 0x18(r3)
0x8007b9d0:  2c000001  cmpwi	r0, 1
0x8007b9d4:  4082000c  bne	0x8007b9e0
0x8007b9d8:  c0030004  lfs	f0, 4(r3)
0x8007b9dc:  48000008  b	0x8007b9e4
0x8007b9e0:  c0030008  lfs	f0, 8(r3)
0x8007b9e4:  d003000c  stfs	f0, 0xc(r3)
0x8007b9e8:  4e800020  blr	

/* 0x800325c4  size=16  callers=87  strings=[] */
0x800325c4:  7c641b78  mr	r4, r3
0x800325c8:  806d9590  lwz	r3, -0x6a70(r13)
0x800325cc:  38a00004  li	r5, 4
0x800325d0:  480de2b8  b	0x80110888

/* 0x80136108  size=24  callers=85  strings=[] */
0x80136108:  880da348  lbz	r0, -0x5cb8(r13)
0x8013610c:  2c000000  cmpwi	r0, 0
0x80136110:  4c820020  bnelr	
0x80136114:  4cc63182  crclr	cr1eq
0x80136118:  4bffff04  b	0x8013601c
0x8013611c:  4e800020  blr	

/* 0x800e67e8  size=4  callers=85  strings=[] */
0x800e67e8:  9421ff70  stwu	r1, -0x90(r1)

/* 0x801619b8  size=4  callers=81  strings=[] */
0x801619b8:  930bffe0  stw	r24, -0x20(r11)

/* 0x80161a04  size=4  callers=81  strings=[] */
0x80161a04:  830bffe0  lwz	r24, -0x20(r11)

/* 0x800ee6f4  size=4  callers=78  strings=[] */
0x800ee6f4:  9421ffb0  stwu	r1, -0x50(r1)

/* 0x80129ca0  size=80  callers=75  strings=[] */
0x80129ca0:  9421ff90  stwu	r1, -0x70(r1)
0x80129ca4:  40860024  bne	cr1, 0x80129cc8
0x80129ca8:  d8210028  stfd	f1, 0x28(r1)
0x80129cac:  d8410030  stfd	f2, 0x30(r1)
0x80129cb0:  d8610038  stfd	f3, 0x38(r1)
0x80129cb4:  d8810040  stfd	f4, 0x40(r1)
0x80129cb8:  d8a10048  stfd	f5, 0x48(r1)
0x80129cbc:  d8c10050  stfd	f6, 0x50(r1)
0x80129cc0:  d8e10058  stfd	f7, 0x58(r1)
0x80129cc4:  d9010060  stfd	f8, 0x60(r1)
0x80129cc8:  90610008  stw	r3, 8(r1)
0x80129ccc:  9081000c  stw	r4, 0xc(r1)
0x80129cd0:  90a10010  stw	r5, 0x10(r1)
0x80129cd4:  90c10014  stw	r6, 0x14(r1)
0x80129cd8:  90e10018  stw	r7, 0x18(r1)
0x80129cdc:  9101001c  stw	r8, 0x1c(r1)
0x80129ce0:  91210020  stw	r9, 0x20(r1)
0x80129ce4:  91410024  stw	r10, 0x24(r1)
0x80129ce8:  38210070  addi	r1, r1, 0x70
0x80129cec:  4e800020  blr	

/* 0x800d7a14  size=36  callers=75  strings=[] */
0x800d7a14:  fc000834  frsqrte	f0, f1
0x800d7a18:  c04293a8  lfs	f2, -0x6c58(r2)
0x800d7a1c:  c06293ac  lfs	f3, -0x6c54(r2)
0x800d7a20:  ec800032  fmuls	f4, f0, f0
0x800d7a24:  ec0000b2  fmuls	f0, f0, f2
0x800d7a28:  ec84187c  fnmsubs	f4, f4, f1, f3
0x800d7a2c:  ec040032  fmuls	f0, f4, f0
0x800d7a30:  fc200090  fmr	f1, f0
0x800d7a34:  4e800020  blr	

/* 0x80031170  size=4  callers=75  strings=[] */
0x80031170:  9421ffd0  stwu	r1, -0x30(r1)

/* 0x800313f4  size=4  callers=72  strings=[] */
0x800313f4:  9421ff80  stwu	r1, -0x80(r1)

/* 0x801619b4  size=4  callers=71  strings=[] */
0x801619b4:  92ebffdc  stw	r23, -0x24(r11)

/* 0x80161a00  size=4  callers=71  strings=[] */
0x80161a00:  82ebffdc  lwz	r23, -0x24(r11)

/* 0x8003a21c  size=4  callers=70  strings=[] */
0x8003a21c:  9421ffe0  stwu	r1, -0x20(r1)

/* 0x800ee7c8  size=4  callers=69  strings=[] */
0x800ee7c8:  9421ffe0  stwu	r1, -0x20(r1)

/* 0x800fbb1c  size=416  callers=68  strings=[] */
0x800fbb1c:  3804fff7  addi	r0, r4, -9
0x800fbb20:  81029548  lwz	r8, -0x6ab8(r2)
0x800fbb24:  28000010  cmplwi	r0, 0x10
0x800fbb28:  5464103a  slwi	r4, r3, 2
0x800fbb2c:  7d082214  add	r8, r8, r4
0x800fbb30:  4181015c  bgt	0x800fbc8c
0x800fbb34:  3c80801c  lis	r4, -0x7fe4
0x800fbb38:  5400103a  slwi	r0, r0, 2
0x800fbb3c:  38847dc8  addi	r4, r4, 0x7dc8
0x800fbb40:  7c84002e  lwzx	r4, r4, r0
0x800fbb44:  7c8903a6  mtctr	r4
0x800fbb48:  4e800420  bctr	
0x800fbb4c:  8008001c  lwz	r0, 0x1c(r8)
0x800fbb50:  50a007fe  rlwimi	r0, r5, 0, 0x1f, 0x1f
0x800fbb54:  50c00f3c  rlwimi	r0, r6, 1, 0x1c, 0x1e
0x800fbb58:  50e025f6  rlwimi	r0, r7, 4, 0x17, 0x1b
0x800fbb5c:  9008001c  stw	r0, 0x1c(r8)
0x800fbb60:  4800012c  b	0x800fbc8c
0x800fbb64:  8008001c  lwz	r0, 0x1c(r8)
0x800fbb68:  50c054ea  rlwimi	r0, r6, 0xa, 0x13, 0x15
0x800fbb6c:  2c050002  cmpwi	r5, 2
0x800fbb70:  9008001c  stw	r0, 0x1c(r8)
0x800fbb74:  40820018  bne	0x800fbb8c
0x800fbb78:  8008001c  lwz	r0, 0x1c(r8)
0x800fbb7c:  60000200  ori	r0, r0, 0x200
0x800fbb80:  64008000  oris	r0, r0, 0x8000
0x800fbb84:  9008001c  stw	r0, 0x1c(r8)
0x800fbb88:  48000104  b	0x800fbc8c
0x800fbb8c:  8008001c  lwz	r0, 0x1c(r8)
0x800fbb90:  50a04dac  rlwimi	r0, r5, 9, 0x16, 0x16
0x800fbb94:  5400007e  clrlwi	r0, r0, 1
0x800fbb98:  9008001c  stw	r0, 0x1c(r8)
0x800fbb9c:  480000f0  b	0x800fbc8c
0x800fbba0:  8008001c  lwz	r0, 0x1c(r8)
0x800fbba4:  50a06ca4  rlwimi	r0, r5, 0xd, 0x12, 0x12
0x800fbba8:  50c073e2  rlwimi	r0, r6, 0xe, 0xf, 0x11
0x800fbbac:  9008001c  stw	r0, 0x1c(r8)
0x800fbbb0:  480000dc  b	0x800fbc8c
0x800fbbb4:  8008001c  lwz	r0, 0x1c(r8)
0x800fbbb8:  50a08b9c  rlwimi	r0, r5, 0x11, 0xe, 0xe
0x800fbbbc:  50c092da  rlwimi	r0, r6, 0x12, 0xb, 0xd
0x800fbbc0:  9008001c  stw	r0, 0x1c(r8)
0x800fbbc4:  480000c8  b	0x800fbc8c
0x800fbbc8:  8008001c  lwz	r0, 0x1c(r8)
0x800fbbcc:  50a0aa94  rlwimi	r0, r5, 0x15, 0xa, 0xa
0x800fbbd0:  50c0b1d2  rlwimi	r0, r6, 0x16, 7, 9
0x800fbbd4:  50e0c88c  rlwimi	r0, r7, 0x19, 2, 6
0x800fbbd8:  9008001c  stw	r0, 0x1c(r8)
0x800fbbdc:  480000b0  b	0x800fbc8c
0x800fbbe0:  8008003c  lwz	r0, 0x3c(r8)
0x800fbbe4:  50a007fe  rlwimi	r0, r5, 0, 0x1f, 0x1f
0x800fbbe8:  50c00f3c  rlwimi	r0, r6, 1, 0x1c, 0x1e
0x800fbbec:  50e025f6  rlwimi	r0, r7, 4, 0x17, 0x1b
0x800fbbf0:  9008003c  stw	r0, 0x3c(r8)
0x800fbbf4:  48000098  b	0x800fbc8c
0x800fbbf8:  8008003c  lwz	r0, 0x3c(r8)
0x800fbbfc:  50a04dac  rlwimi	r0, r5, 9, 0x16, 0x16
0x800fbc00:  50c054ea  rlwimi	r0, r6, 0xa, 0x13, 0x15
0x800fbc04:  50e06ba4  rlwimi	r0, r7, 0xd, 0xe, 0x12
0x800fbc08:  9008003c  stw	r0, 0x3c(r8)
0x800fbc0c:  48000080  b	0x800fbc8c
0x800fbc10:  8008003c  lwz	r0, 0x3c(r8)
0x800fbc14:  50a0935a  rlwimi	r0, r5, 0x12, 0xd, 0xd
0x800fbc18:  50c09a98  rlwimi	r0, r6, 0x13, 0xa, 0xc
0x800fbc1c:  50e0b152  rlwimi	r0, r7, 0x16, 5, 9
0x800fbc20:  9008003c  stw	r0, 0x3c(r8)
0x800fbc24:  48000068  b	0x800fbc8c
0x800fbc28:  8008003c  lwz	r0, 0x3c(r8)
0x800fbc2c:  50a0d908  rlwimi	r0, r5, 0x1b, 4, 4
0x800fbc30:  50c0e046  rlwimi	r0, r6, 0x1c, 1, 3
0x800fbc34:  9008003c  stw	r0, 0x3c(r8)
0x800fbc38:  8008005c  lwz	r0, 0x5c(r8)
0x800fbc3c:  50e006fe  rlwimi	r0, r7, 0, 0x1b, 0x1f
0x800fbc40:  9008005c  stw	r0, 0x5c(r8)
0x800fbc44:  48000048  b	0x800fbc8c
0x800fbc48:  8008005c  lwz	r0, 0x5c(r8)
0x800fbc4c:  50a02eb4  rlwimi	r0, r5, 5, 0x1a, 0x1a
0x800fbc50:  50c035f2  rlwimi	r0, r6, 6, 0x17, 0x19
0x800fbc54:  50e04cac  rlwimi	r0, r7, 9, 0x12, 0x16
0x800fbc58:  9008005c  stw	r0, 0x5c(r8)
0x800fbc5c:  48000030  b	0x800fbc8c
0x800fbc60:  8008005c  lwz	r0, 0x5c(r8)
0x800fbc64:  50a07462  rlwimi	r0, r5, 0xe, 0x11, 0x11
0x800fbc68:  50c07ba0  rlwimi	r0, r6, 0xf, 0xe, 0x10
0x800fbc6c:  50e0925a  rlwimi	r0, r7, 0x12, 9, 0xd
0x800fbc70:  9008005c  stw	r0, 0x5c(r8)
0x800fbc74:  48000018  b	0x800fbc8c
0x800fbc78:  8008005c  lwz	r0, 0x5c(r8)
0x800fbc7c:  50a0ba10  rlwimi	r0, r5, 0x17, 8, 8
0x800fbc80:  50c0c14e  rlwimi	r0, r6, 0x18, 5, 7
0x800fbc84:  50e0d808  rlwimi	r0, r7, 0x1b, 0, 4
0x800fbc88:  9008005c  stw	r0, 0x5c(r8)
0x800fbc8c:  80a29548  lwz	r5, -0x6ab8(r2)
0x800fbc90:  5460063e  clrlwi	r0, r3, 0x18
0x800fbc94:  38600001  li	r3, 1
0x800fbc98:  808505fc  lwz	r4, 0x5fc(r5)
0x800fbc9c:  7c600030  slw	r0, r3, r0
0x800fbca0:  5400063e  clrlwi	r0, r0, 0x18
0x800fbca4:  60830010  ori	r3, r4, 0x10
0x800fbca8:  906505fc  stw	r3, 0x5fc(r5)
0x800fbcac:  886505fb  lbz	r3, 0x5fb(r5)
0x800fbcb0:  7c600378  or	r0, r3, r0
0x800fbcb4:  980505fb  stb	r0, 0x5fb(r5)
0x800fbcb8:  4e800020  blr	

/* 0x800fb258  size=612  callers=67  strings=[] */
0x800fb258:  28030019  cmplwi	r3, 0x19
0x800fb25c:  41810210  bgt	0x800fb46c
0x800fb260:  3ca0801c  lis	r5, -0x7fe4
0x800fb264:  5460103a  slwi	r0, r3, 2
0x800fb268:  38a57c90  addi	r5, r5, 0x7c90
0x800fb26c:  7ca5002e  lwzx	r5, r5, r0
0x800fb270:  7ca903a6  mtctr	r5
0x800fb274:  4e800420  bctr	
0x800fb278:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb27c:  80030014  lwz	r0, 0x14(r3)
0x800fb280:  508007fe  rlwimi	r0, r4, 0, 0x1f, 0x1f
0x800fb284:  90030014  stw	r0, 0x14(r3)
0x800fb288:  480001e4  b	0x800fb46c
0x800fb28c:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb290:  80030014  lwz	r0, 0x14(r3)
0x800fb294:  50800fbc  rlwimi	r0, r4, 1, 0x1e, 0x1e
0x800fb298:  90030014  stw	r0, 0x14(r3)
0x800fb29c:  480001d0  b	0x800fb46c
0x800fb2a0:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb2a4:  80030014  lwz	r0, 0x14(r3)
0x800fb2a8:  5080177a  rlwimi	r0, r4, 2, 0x1d, 0x1d
0x800fb2ac:  90030014  stw	r0, 0x14(r3)
0x800fb2b0:  480001bc  b	0x800fb46c
0x800fb2b4:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb2b8:  80030014  lwz	r0, 0x14(r3)
0x800fb2bc:  50801f38  rlwimi	r0, r4, 3, 0x1c, 0x1c
0x800fb2c0:  90030014  stw	r0, 0x14(r3)
0x800fb2c4:  480001a8  b	0x800fb46c
0x800fb2c8:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb2cc:  80030014  lwz	r0, 0x14(r3)
0x800fb2d0:  508026f6  rlwimi	r0, r4, 4, 0x1b, 0x1b
0x800fb2d4:  90030014  stw	r0, 0x14(r3)
0x800fb2d8:  48000194  b	0x800fb46c
0x800fb2dc:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb2e0:  80030014  lwz	r0, 0x14(r3)
0x800fb2e4:  50802eb4  rlwimi	r0, r4, 5, 0x1a, 0x1a
0x800fb2e8:  90030014  stw	r0, 0x14(r3)
0x800fb2ec:  48000180  b	0x800fb46c
0x800fb2f0:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb2f4:  80030014  lwz	r0, 0x14(r3)
0x800fb2f8:  50803672  rlwimi	r0, r4, 6, 0x19, 0x19
0x800fb2fc:  90030014  stw	r0, 0x14(r3)
0x800fb300:  4800016c  b	0x800fb46c
0x800fb304:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb308:  80030014  lwz	r0, 0x14(r3)
0x800fb30c:  50803e30  rlwimi	r0, r4, 7, 0x18, 0x18
0x800fb310:  90030014  stw	r0, 0x14(r3)
0x800fb314:  48000158  b	0x800fb46c
0x800fb318:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb31c:  80030014  lwz	r0, 0x14(r3)
0x800fb320:  508045ee  rlwimi	r0, r4, 8, 0x17, 0x17
0x800fb324:  90030014  stw	r0, 0x14(r3)
0x800fb328:  48000144  b	0x800fb46c
0x800fb32c:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb330:  80030014  lwz	r0, 0x14(r3)
0x800fb334:  50804d6c  rlwimi	r0, r4, 9, 0x15, 0x16
0x800fb338:  90030014  stw	r0, 0x14(r3)
0x800fb33c:  48000130  b	0x800fb46c
0x800fb340:  2c040000  cmpwi	r4, 0
0x800fb344:  41820020  beq	0x800fb364
0x800fb348:  80a29548  lwz	r5, -0x6ab8(r2)
0x800fb34c:  38600001  li	r3, 1
0x800fb350:  38000000  li	r0, 0
0x800fb354:  98650524  stb	r3, 0x524(r5)
0x800fb358:  98050525  stb	r0, 0x525(r5)
0x800fb35c:  90850520  stw	r4, 0x520(r5)
0x800fb360:  4800010c  b	0x800fb46c
0x800fb364:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb368:  38000000  li	r0, 0
0x800fb36c:  98030524  stb	r0, 0x524(r3)
0x800fb370:  480000fc  b	0x800fb46c
0x800fb374:  2c040000  cmpwi	r4, 0
0x800fb378:  41820020  beq	0x800fb398
0x800fb37c:  80a29548  lwz	r5, -0x6ab8(r2)
0x800fb380:  38600001  li	r3, 1
0x800fb384:  38000000  li	r0, 0
0x800fb388:  98650525  stb	r3, 0x525(r5)
0x800fb38c:  98050524  stb	r0, 0x524(r5)
0x800fb390:  90850520  stw	r4, 0x520(r5)
0x800fb394:  480000d8  b	0x800fb46c
0x800fb398:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb39c:  38000000  li	r0, 0
0x800fb3a0:  98030525  stb	r0, 0x525(r3)
0x800fb3a4:  480000c8  b	0x800fb46c
0x800fb3a8:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb3ac:  80030014  lwz	r0, 0x14(r3)
0x800fb3b0:  50806c64  rlwimi	r0, r4, 0xd, 0x11, 0x12
0x800fb3b4:  90030014  stw	r0, 0x14(r3)
0x800fb3b8:  480000b4  b	0x800fb46c
0x800fb3bc:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb3c0:  80030014  lwz	r0, 0x14(r3)
0x800fb3c4:  50807be0  rlwimi	r0, r4, 0xf, 0xf, 0x10
0x800fb3c8:  90030014  stw	r0, 0x14(r3)
0x800fb3cc:  480000a0  b	0x800fb46c
0x800fb3d0:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb3d4:  80030018  lwz	r0, 0x18(r3)
0x800fb3d8:  508007be  rlwimi	r0, r4, 0, 0x1e, 0x1f
0x800fb3dc:  90030018  stw	r0, 0x18(r3)
0x800fb3e0:  4800008c  b	0x800fb46c
0x800fb3e4:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb3e8:  80030018  lwz	r0, 0x18(r3)
0x800fb3ec:  5080173a  rlwimi	r0, r4, 2, 0x1c, 0x1d
0x800fb3f0:  90030018  stw	r0, 0x18(r3)
0x800fb3f4:  48000078  b	0x800fb46c
0x800fb3f8:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb3fc:  80030018  lwz	r0, 0x18(r3)
0x800fb400:  508026b6  rlwimi	r0, r4, 4, 0x1a, 0x1b
0x800fb404:  90030018  stw	r0, 0x18(r3)
0x800fb408:  48000064  b	0x800fb46c
0x800fb40c:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb410:  80030018  lwz	r0, 0x18(r3)
0x800fb414:  50803632  rlwimi	r0, r4, 6, 0x18, 0x19
0x800fb418:  90030018  stw	r0, 0x18(r3)
0x800fb41c:  48000050  b	0x800fb46c
0x800fb420:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb424:  80030018  lwz	r0, 0x18(r3)
0x800fb428:  508045ae  rlwimi	r0, r4, 8, 0x16, 0x17
0x800fb42c:  90030018  stw	r0, 0x18(r3)
0x800fb430:  4800003c  b	0x800fb46c
0x800fb434:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb438:  80030018  lwz	r0, 0x18(r3)
0x800fb43c:  5080552a  rlwimi	r0, r4, 0xa, 0x14, 0x15
0x800fb440:  90030018  stw	r0, 0x18(r3)
0x800fb444:  48000028  b	0x800fb46c
0x800fb448:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb44c:  80030018  lwz	r0, 0x18(r3)
0x800fb450:  508064a6  rlwimi	r0, r4, 0xc, 0x12, 0x13
0x800fb454:  90030018  stw	r0, 0x18(r3)
0x800fb458:  48000014  b	0x800fb46c
0x800fb45c:  80629548  lwz	r3, -0x6ab8(r2)
0x800fb460:  80030018  lwz	r0, 0x18(r3)
0x800fb464:  50807422  rlwimi	r0, r4, 0xe, 0x10, 0x11
0x800fb468:  90030018  stw	r0, 0x18(r3)
0x800fb46c:  80829548  lwz	r4, -0x6ab8(r2)
0x800fb470:  88040524  lbz	r0, 0x524(r4)
0x800fb474:  2c000000  cmpwi	r0, 0
0x800fb478:  40820010  bne	0x800fb488
0x800fb47c:  88040525  lbz	r0, 0x525(r4)
0x800fb480:  2c000000  cmpwi	r0, 0
0x800fb484:  4182001c  beq	0x800fb4a0
0x800fb488:  80a29548  lwz	r5, -0x6ab8(r2)
0x800fb48c:  80050520  lwz	r0, 0x520(r5)
0x800fb490:  80650014  lwz	r3, 0x14(r5)
0x800fb494:  50035ce8  rlwimi	r3, r0, 0xb, 0x13, 0x14
0x800fb498:  90650014  stw	r3, 0x14(r5)
0x800fb49c:  48000010  b	0x800fb4ac
0x800fb4a0:  80040014  lwz	r0, 0x14(r4)
0x800fb4a4:  54000564  rlwinm	r0, r0, 0, 0x15, 0x12
0x800fb4a8:  90040014  stw	r0, 0x14(r4)
0x800fb4ac:  800405fc  lwz	r0, 0x5fc(r4)
0x800fb4b0:  60000008  ori	r0, r0, 8
0x800fb4b4:  900405fc  stw	r0, 0x5fc(r4)
0x800fb4b8:  4e800020  blr	

/* 0x801366a4  size=4  callers=66  strings=[] */
0x801366a4:  9421ffe0  stwu	r1, -0x20(r1)

/* 0x800ee7a8  size=24  callers=65  strings=[] */
0x800ee7a8:  7c6d42e6  mftbu	r3
0x800ee7ac:  7c8c42e6  mftb	r4, 0x10c
0x800ee7b0:  7cad42e6  mftbu	r5
0x800ee7b4:  7c032800  cmpw	r3, r5
0x800ee7b8:  4082fff0  bne	0x800ee7a8
0x800ee7bc:  4e800020  blr	

/* 0x800d4eb8  size=12  callers=65  strings=[] */
0x800d4eb8:  d023002c  stfs	f1, 0x2c(r3)
0x800d4ebc:  d0430030  stfs	f2, 0x30(r3)
0x800d4ec0:  4e800020  blr	

/* 0x80038e0c  size=12  callers=64  strings=[] */
0x80038e0c:  90830080  stw	r4, 0x80(r3)
0x80038e10:  90830084  stw	r4, 0x84(r3)
0x80038e14:  4e800020  blr	

/* 0x801612ec  size=28  callers=63  strings=[] */
0x801612ec:  3883ffff  addi	r4, r3, -1
0x801612f0:  3860ffff  li	r3, -1
0x801612f4:  8c040001  lbzu	r0, 1(r4)
0x801612f8:  38630001  addi	r3, r3, 1
0x801612fc:  2c000000  cmpwi	r0, 0
0x80161300:  4082fff4  bne	0x801612f4
0x80161304:  4e800020  blr	

/* 0x800e58e4  size=48  callers=61  strings=[] */
0x800e58e4:  28040000  cmplwi	r4, 0
0x800e58e8:  4c810020  blelr	
0x800e58ec:  546506fe  clrlwi	r5, r3, 0x1b
0x800e58f0:  7c842a14  add	r4, r4, r5
0x800e58f4:  3884001f  addi	r4, r4, 0x1f
0x800e58f8:  5484d97e  srwi	r4, r4, 5
0x800e58fc:  7c8903a6  mtctr	r4
0x800e5900:  7c0018ac  dcbf	0, r3
0x800e5904:  38630020  addi	r3, r3, 0x20
0x800e5908:  4200fff8  bdnz	0x800e5900
0x800e590c:  44000002  sc	
0x800e5910:  4e800020  blr	

/* 0x800ee7c0  size=8  callers=61  strings=[] */
0x800ee7c0:  7c6c42e6  mftb	r3, 0x10c
0x800ee7c4:  4e800020  blr	

/* 0x8016189c  size=92  callers=60  strings=[] */
0x8016189c:  9421fff0  stwu	r1, -0x10(r1)
0x801618a0:  3c808019  lis	r4, -0x7fe7
0x801618a4:  388479b0  addi	r4, r4, 0x79b0
0x801618a8:  38600000  li	r3, 0
0x801618ac:  c8040000  lfd	f0, 0(r4)
0x801618b0:  c8640008  lfd	f3, 8(r4)
0x801618b4:  c8840010  lfd	f4, 0x10(r4)
0x801618b8:  fc010000  fcmpu	cr0, f1, f0
0x801618bc:  ff011800  fcmpu	cr6, f1, f3
0x801618c0:  41800030  blt	0x801618f0
0x801618c4:  3863ffff  addi	r3, r3, -1
0x801618c8:  40980028  bge	cr6, 0x801618f0
0x801618cc:  ff812000  fcmpu	cr7, f1, f4
0x801618d0:  fc400890  fmr	f2, f1
0x801618d4:  419c0008  blt	cr7, 0x801618dc
0x801618d8:  fc412028  fsub	f2, f1, f4
0x801618dc:  fc40101e  fctiwz	f2, f2
0x801618e0:  d8410008  stfd	f2, 8(r1)
0x801618e4:  8061000c  lwz	r3, 0xc(r1)
0x801618e8:  419c0008  blt	cr7, 0x801618f0
0x801618ec:  3c638000  addis	r3, r3, -0x8000
0x801618f0:  38210010  addi	r1, r1, 0x10
0x801618f4:  4e800020  blr	

/* 0x800f9254  size=84  callers=59  strings=[] */
0x800f9254:  e0040000  .byte	0xe0, 0x04, 0x00, 0x00
0x800f9258:  e0430000  .byte	0xe0, 0x43, 0x00, 0x00
0x800f925c:  e0248008  .byte	0xe0, 0x24, 0x80, 0x08
0x800f9260:  10820032  .byte	0x10, 0x82, 0x00, 0x32
0x800f9264:  e0630008  .byte	0xe0, 0x63, 0x00, 0x08
0x800f9268:  10a3207a  .byte	0x10, 0xa3, 0x20, 0x7a
0x800f926c:  e1030010  .byte	0xe1, 0x03, 0x00, 0x10
0x800f9270:  10c52994  .byte	0x10, 0xc5, 0x29, 0x94
0x800f9274:  e1230018  .byte	0xe1, 0x23, 0x00, 0x18
0x800f9278:  11480032  .byte	0x11, 0x48, 0x00, 0x32
0x800f927c:  f0c58000  xsaddsp	f6, f5, f16
0x800f9280:  1169507a  .byte	0x11, 0x69, 0x50, 0x7a
0x800f9284:  e0430020  .byte	0xe0, 0x43, 0x00, 0x20
0x800f9288:  118b5b14  .byte	0x11, 0x8b, 0x5b, 0x14
0x800f928c:  e0630028  .byte	0xe0, 0x63, 0x00, 0x28
0x800f9290:  10820032  .byte	0x10, 0x82, 0x00, 0x32
0x800f9294:  f1858004  xsaddsp	f12, v5, f16
0x800f9298:  10a3207a  .byte	0x10, 0xa3, 0x20, 0x7a
0x800f929c:  10c52994  .byte	0x10, 0xc5, 0x29, 0x94
0x800f92a0:  f0c58008  xsmaddasp	f6, f5, f16
0x800f92a4:  4e800020  blr	

/* 0x800d56d0  size=8  callers=59  strings=[] */
0x800d56d0:  d023004c  stfs	f1, 0x4c(r3)
0x800d56d4:  4e800020  blr	

/* 0x800d4868  size=352  callers=57  strings=[] */
0x800d4868:  80030020  lwz	r0, 0x20(r3)
0x800d486c:  88e40000  lbz	r7, 0(r4)
0x800d4870:  88c40001  lbz	r6, 1(r4)
0x800d4874:  2c000001  cmpwi	r0, 1
0x800d4878:  88a40002  lbz	r5, 2(r4)
0x800d487c:  88040003  lbz	r0, 3(r4)
0x800d4880:  98e30018  stb	r7, 0x18(r3)
0x800d4884:  98c30019  stb	r6, 0x19(r3)
0x800d4888:  98a3001a  stb	r5, 0x1a(r3)
0x800d488c:  9803001b  stb	r0, 0x1b(r3)
0x800d4890:  98e30008  stb	r7, 8(r3)
0x800d4894:  98c30009  stb	r6, 9(r3)
0x800d4898:  98a3000a  stb	r5, 0xa(r3)
0x800d489c:  9803000b  stb	r0, 0xb(r3)
0x800d48a0:  4182000c  beq	0x800d48ac
0x800d48a4:  38a30018  addi	r5, r3, 0x18
0x800d48a8:  48000008  b	0x800d48b0
0x800d48ac:  38a3001c  addi	r5, r3, 0x1c
0x800d48b0:  88850000  lbz	r4, 0(r5)
0x800d48b4:  80030020  lwz	r0, 0x20(r3)
0x800d48b8:  9883000c  stb	r4, 0xc(r3)
0x800d48bc:  2c000002  cmpwi	r0, 2
0x800d48c0:  88050001  lbz	r0, 1(r5)
0x800d48c4:  9803000d  stb	r0, 0xd(r3)
0x800d48c8:  88050002  lbz	r0, 2(r5)
0x800d48cc:  9803000e  stb	r0, 0xe(r3)
0x800d48d0:  88050003  lbz	r0, 3(r5)
0x800d48d4:  9803000f  stb	r0, 0xf(r3)
0x800d48d8:  4182000c  beq	0x800d48e4
0x800d48dc:  38a30018  addi	r5, r3, 0x18
0x800d48e0:  48000008  b	0x800d48e8
0x800d48e4:  38a3001c  addi	r5, r3, 0x1c
0x800d48e8:  88850000  lbz	r4, 0(r5)
0x800d48ec:  80030020  lwz	r0, 0x20(r3)
0x800d48f0:  98830010  stb	r4, 0x10(r3)
0x800d48f4:  2c000000  cmpwi	r0, 0
0x800d48f8:  88050001  lbz	r0, 1(r5)
0x800d48fc:  98030011  stb	r0, 0x11(r3)
0x800d4900:  88050002  lbz	r0, 2(r5)
0x800d4904:  98030012  stb	r0, 0x12(r3)
0x800d4908:  88050003  lbz	r0, 3(r5)
0x800d490c:  98030013  stb	r0, 0x13(r3)
0x800d4910:  4082000c  bne	0x800d491c
0x800d4914:  39030018  addi	r8, r3, 0x18
0x800d4918:  48000008  b	0x800d4920
0x800d491c:  3903001c  addi	r8, r3, 0x1c
0x800d4920:  88a80000  lbz	r5, 0(r8)
0x800d4924:  3c808081  lis	r4, -0x7f7f
0x800d4928:  39448081  addi	r10, r4, -0x7f7f
0x800d492c:  8803000b  lbz	r0, 0xb(r3)
0x800d4930:  98a30014  stb	r5, 0x14(r3)
0x800d4934:  88e30042  lbz	r7, 0x42(r3)
0x800d4938:  88a80001  lbz	r5, 1(r8)
0x800d493c:  8883000f  lbz	r4, 0xf(r3)
0x800d4940:  7cc039d6  mullw	r6, r0, r7
0x800d4944:  98a30015  stb	r5, 0x15(r3)
0x800d4948:  88030013  lbz	r0, 0x13(r3)
0x800d494c:  88a80002  lbz	r5, 2(r8)
0x800d4950:  98a30016  stb	r5, 0x16(r3)
0x800d4954:  7c8439d6  mullw	r4, r4, r7
0x800d4958:  89080003  lbz	r8, 3(r8)
0x800d495c:  7ca039d6  mullw	r5, r0, r7
0x800d4960:  7c0839d6  mullw	r0, r8, r7
0x800d4964:  7d0a3096  mulhw	r8, r10, r6
0x800d4968:  7cea2096  mulhw	r7, r10, r4
0x800d496c:  7cc83214  add	r6, r8, r6
0x800d4970:  7cc83e70  srawi	r8, r6, 7
0x800d4974:  7cca2896  mulhw	r6, r10, r5
0x800d4978:  55090ffe  srwi	r9, r8, 0x1f
0x800d497c:  7c872214  add	r4, r7, r4
0x800d4980:  7d084a14  add	r8, r8, r9
0x800d4984:  7c873e70  srawi	r7, r4, 7
0x800d4988:  9903000b  stb	r8, 0xb(r3)
0x800d498c:  7ca62a14  add	r5, r6, r5
0x800d4990:  54e60ffe  srwi	r6, r7, 0x1f
0x800d4994:  7c8a0096  mulhw	r4, r10, r0
0x800d4998:  7ce73214  add	r7, r7, r6
0x800d499c:  7ca53e70  srawi	r5, r5, 7
0x800d49a0:  98e3000f  stb	r7, 0xf(r3)
0x800d49a4:  54a60ffe  srwi	r6, r5, 0x1f
0x800d49a8:  7c040214  add	r0, r4, r0
0x800d49ac:  7c003e70  srawi	r0, r0, 7
0x800d49b0:  7ca53214  add	r5, r5, r6
0x800d49b4:  54040ffe  srwi	r4, r0, 0x1f
0x800d49b8:  98a30013  stb	r5, 0x13(r3)
0x800d49bc:  7c002214  add	r0, r0, r4
0x800d49c0:  98030017  stb	r0, 0x17(r3)
0x800d49c4:  4e800020  blr	

/* 0x80161308  size=200  callers=55  strings=[] */
0x80161308:  88e30000  lbz	r7, 0(r3)
0x8016130c:  2c040003  cmpwi	r4, 3
0x80161310:  7c661b78  mr	r6, r3
0x80161314:  38000008  li	r0, 8
0x80161318:  7ce70774  extsb	r7, r7
0x8016131c:  39000004  li	r8, 4
0x80161320:  39200001  li	r9, 1
0x80161324:  38a00000  li	r5, 0
0x80161328:  39400000  li	r10, 0
0x8016132c:  39600004  li	r11, 4
0x80161330:  4082001c  bne	0x8016134c
0x80161334:  88e30001  lbz	r7, 1(r3)
0x80161338:  38c30001  addi	r6, r3, 1
0x8016133c:  39000008  li	r8, 8
0x80161340:  39400020  li	r10, 0x20
0x80161344:  7ce70774  extsb	r7, r7
0x80161348:  39600008  li	r11, 8
0x8016134c:  2c040002  cmpwi	r4, 2
0x80161350:  4082001c  bne	0x8016136c
0x80161354:  54e007ff  clrlwi.	r0, r7, 0x1f
0x80161358:  39000008  li	r8, 8
0x8016135c:  38000007  li	r0, 7
0x80161360:  41820008  beq	0x80161368
0x80161364:  38a00001  li	r5, 1
0x80161368:  39200002  li	r9, 2
0x8016136c:  7c070000  cmpw	r7, r0
0x80161370:  40800024  bge	0x80161394
0x80161374:  7ce72a14  add	r7, r7, r5
0x80161378:  80630008  lwz	r3, 8(r3)
0x8016137c:  7ca759d6  mullw	r5, r7, r11
0x80161380:  7c074a14  add	r0, r7, r9
0x80161384:  7c635214  add	r3, r3, r10
0x80161388:  98060000  stb	r0, 0(r6)
0x8016138c:  7ca51a14  add	r5, r5, r3
0x80161390:  4800002c  b	0x801613bc
0x80161394:  38a00008  li	r5, 8
0x80161398:  3808ffff  addi	r0, r8, -1
0x8016139c:  98a60000  stb	r5, 0(r6)
0x801613a0:  7c0600f8  nor	r6, r0, r0
0x801613a4:  80030004  lwz	r0, 4(r3)
0x801613a8:  7ca80214  add	r5, r8, r0
0x801613ac:  3805ffff  addi	r0, r5, -1
0x801613b0:  7cc50038  and	r5, r6, r0
0x801613b4:  7c054214  add	r0, r5, r8
0x801613b8:  90030004  stw	r0, 4(r3)
0x801613bc:  2c040000  cmpwi	r4, 0
0x801613c0:  40820008  bne	0x801613c8
0x801613c4:  80a50000  lwz	r5, 0(r5)
0x801613c8:  7ca32b78  mr	r3, r5
0x801613cc:  4e800020  blr	

/* 0x800ffac4  size=348  callers=54  strings=[] */
0x800ffac4:  54ab062c  rlwinm	r11, r5, 0, 0x18, 0x16
0x800ffac8:  38e00008  li	r7, 8
0x800ffacc:  54690ffe  srwi	r9, r3, 0x1f
0x800ffad0:  81429548  lwz	r10, -0x6ab8(r2)
0x800ffad4:  7c075810  subfc	r0, r7, r11
0x800ffad8:  5468103a  slwi	r8, r3, 2
0x800ffadc:  7c070194  addze	r0, r7
0x800ffae0:  7d291a14  add	r9, r9, r3
0x800ffae4:  7cea4214  add	r7, r10, r8
0x800ffae8:  2c040008  cmpwi	r4, 8
0x800ffaec:  5528083a  rlwinm	r8, r9, 1, 0, 0x1d
0x800ffaf0:  20000008  subfic	r0, r0, 8
0x800ffaf4:  90a705a4  stw	r5, 0x5a4(r7)
0x800ffaf8:  7cea4214  add	r7, r10, r8
0x800ffafc:  7d690078  andc	r9, r11, r0
0x800ffb00:  41800020  blt	0x800ffb20
0x800ffb04:  38000001  li	r0, 1
0x800ffb08:  810a05e8  lwz	r8, 0x5e8(r10)
0x800ffb0c:  7c001830  slw	r0, r0, r3
0x800ffb10:  38800000  li	r4, 0
0x800ffb14:  7d000078  andc	r0, r8, r0
0x800ffb18:  900a05e8  stw	r0, 0x5e8(r10)
0x800ffb1c:  48000018  b	0x800ffb34
0x800ffb20:  38000001  li	r0, 1
0x800ffb24:  810a05e8  lwz	r8, 0x5e8(r10)
0x800ffb28:  7c001830  slw	r0, r0, r3
0x800ffb2c:  7d000378  or	r0, r8, r0
0x800ffb30:  900a05e8  stw	r0, 0x5e8(r10)
0x800ffb34:  546007ff  clrlwi.	r0, r3, 0x1f
0x800ffb38:  41820060  beq	0x800ffb98
0x800ffb3c:  81070150  lwz	r8, 0x150(r7)
0x800ffb40:  51286466  rlwimi	r8, r9, 0xc, 0x11, 0x13
0x800ffb44:  50887ba0  rlwimi	r8, r4, 0xf, 0xe, 0x10
0x800ffb48:  2c0600ff  cmpwi	r6, 0xff
0x800ffb4c:  91070150  stw	r8, 0x150(r7)
0x800ffb50:  4082000c  bne	0x800ffb5c
0x800ffb54:  38000007  li	r0, 7
0x800ffb58:  48000014  b	0x800ffb6c
0x800ffb5c:  3c60801d  lis	r3, -0x7fe3
0x800ffb60:  54c0103a  slwi	r0, r6, 2
0x800ffb64:  38638198  addi	r3, r3, -0x7e68
0x800ffb68:  7c03002e  lwzx	r0, r3, r0
0x800ffb6c:  50089a98  rlwimi	r8, r0, 0x13, 0xa, 0xc
0x800ffb70:  2c0500ff  cmpwi	r5, 0xff
0x800ffb74:  91070150  stw	r8, 0x150(r7)
0x800ffb78:  38600000  li	r3, 0
0x800ffb7c:  41820010  beq	0x800ffb8c
0x800ffb80:  54a005ef  rlwinm.	r0, r5, 0, 0x17, 0x17
0x800ffb84:  40820008  bne	0x800ffb8c
0x800ffb88:  38600001  li	r3, 1
0x800ffb8c:  5068935a  rlwimi	r8, r3, 0x12, 0xd, 0xd
0x800ffb90:  91070150  stw	r8, 0x150(r7)
0x800ffb94:  4800005c  b	0x800ffbf0
0x800ffb98:  81070150  lwz	r8, 0x150(r7)
0x800ffb9c:  5128077e  rlwimi	r8, r9, 0, 0x1d, 0x1f
0x800ffba0:  50881eb8  rlwimi	r8, r4, 3, 0x1a, 0x1c
0x800ffba4:  2c0600ff  cmpwi	r6, 0xff
0x800ffba8:  91070150  stw	r8, 0x150(r7)
0x800ffbac:  4082000c  bne	0x800ffbb8
0x800ffbb0:  38000007  li	r0, 7
0x800ffbb4:  48000014  b	0x800ffbc8
0x800ffbb8:  3c60801d  lis	r3, -0x7fe3
0x800ffbbc:  54c0103a  slwi	r0, r6, 2
0x800ffbc0:  38638198  addi	r3, r3, -0x7e68
0x800ffbc4:  7c03002e  lwzx	r0, r3, r0
0x800ffbc8:  50083db0  rlwimi	r8, r0, 7, 0x16, 0x18
0x800ffbcc:  2c0500ff  cmpwi	r5, 0xff
0x800ffbd0:  91070150  stw	r8, 0x150(r7)
0x800ffbd4:  38600000  li	r3, 0
0x800ffbd8:  41820010  beq	0x800ffbe8
0x800ffbdc:  54a005ef  rlwinm.	r0, r5, 0, 0x17, 0x17
0x800ffbe0:  40820008  bne	0x800ffbe8
0x800ffbe4:  38600001  li	r3, 1
0x800ffbe8:  50683672  rlwimi	r8, r3, 6, 0x19, 0x19
0x800ffbec:  91070150  stw	r8, 0x150(r7)
0x800ffbf0:  3c80cc01  lis	r4, -0x33ff
0x800ffbf4:  38000061  li	r0, 0x61
0x800ffbf8:  98048000  stb	r0, -0x8000(r4)
0x800ffbfc:  38000000  li	r0, 0
0x800ffc00:  80a29548  lwz	r5, -0x6ab8(r2)
0x800ffc04:  80670150  lwz	r3, 0x150(r7)
0x800ffc08:  90648000  stw	r3, -0x8000(r4)
0x800ffc0c:  b0050002  sth	r0, 2(r5)
0x800ffc10:  800505fc  lwz	r0, 0x5fc(r5)
0x800ffc14:  60000001  ori	r0, r0, 1
0x800ffc18:  900505fc  stw	r0, 0x5fc(r5)
0x800ffc1c:  4e800020  blr	

/* 0x8016f56c  size=28  callers=54  strings=[] */
0x8016f56c:  3883fffe  addi	r4, r3, -2
0x8016f570:  3860ffff  li	r3, -1
0x8016f574:  a4040002  lhzu	r0, 2(r4)
0x8016f578:  38630001  addi	r3, r3, 1
0x8016f57c:  2c000000  cmpwi	r0, 0
0x8016f580:  4082fff4  bne	0x8016f574
0x8016f584:  4e800020  blr	

/* 0x800c5d30  size=80  callers=53  strings=[] */
0x800c5d30:  80830000  lwz	r4, 0(r3)
0x800c5d34:  2c040000  cmpwi	r4, 0
0x800c5d38:  41820030  beq	0x800c5d68
0x800c5d3c:  80040008  lwz	r0, 8(r4)
0x800c5d40:  7c001840  cmplw	r0, r3
0x800c5d44:  4082000c  bne	0x800c5d50
0x800c5d48:  38000000  li	r0, 0
0x800c5d4c:  90040008  stw	r0, 8(r4)
0x800c5d50:  80830000  lwz	r4, 0(r3)
0x800c5d54:  8004000c  lwz	r0, 0xc(r4)
0x800c5d58:  7c001840  cmplw	r0, r3
0x800c5d5c:  4082000c  bne	0x800c5d68
0x800c5d60:  38000000  li	r0, 0
0x800c5d64:  9004000c  stw	r0, 0xc(r4)
0x800c5d68:  80030000  lwz	r0, 0(r3)
0x800c5d6c:  2c000000  cmpwi	r0, 0
0x800c5d70:  4d820020  beqlr	
0x800c5d74:  38000000  li	r0, 0
0x800c5d78:  90030000  stw	r0, 0(r3)
0x800c5d7c:  4e800020  blr	
