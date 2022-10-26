                      **************************************************************
                      *                          FUNCTION                          *
                      **************************************************************
                      noreturn void __stdcall main_init(bool skip_checksum_che
      void              <VOID>         <RETURN>
      bool              r4:1           skip_checksum_check
      bool              r5:1           param2
      mainJump *        r13:4          jump_address
      bool              r0:1           status
                      main_init                                       XREF[1]:     000008c4(c)  
000004e0 7f f8           add        -0x8,r15
000004e2 1f 41           mov.l      skip_checksum_check,@(0x4,r15)
000004e4 2f 50           mov.b      param2,@r15=>Stack[-0x8]
000004e6 b0 44           bsr        timer_init                                       void timer_init(void)
000004e8 ee 01           _mov       #0x1,r14
000004ea 92 4c           mov.w      @(DAT_00000586,pc),r2                            = 0170h
000004ec 42 0b           jsr        @r2=>can_init                                    void can_init(void)
000004ee 00 09           _nop
000004f0 93 4a           mov.w      @(DAT_00000588,pc),r3                            = 041Ch
000004f2 43 0b           jsr        @r3=>FUN_0000041c                                void FUN_0000041c(void)
000004f4 00 09           _nop
000004f6 92 48           mov.w      @(DAT_0000058a,pc),r2                            = 03D4h
000004f8 42 0b           jsr        @r2=>FUN_000003d4                                void FUN_000003d4(void)
000004fa 00 09           _nop
000004fc 53 f1           mov.l      @(0x4,r15),r3
000004fe 23 38           tst        r3,r3
00000500 8b 29           bf         LAB_00000556
00000502 d3 26           mov.l      @(DAT_0000059c,pc),r3                            = 5AA5A55Ah
00000504 d1 26           mov.l      @(->CHECKSUM,pc),r1                              = ffffdffc
00000506 62 12           mov.l      @r1=>CHECKSUM,r2                                 = ??
00000508 32 30           cmp/eq     r3,r2
0000050a 89 06           bt         LAB_0000051a
0000050c b0 50           bsr        FUN_000005b0                                     bool FUN_000005b0(ushort numLoops)
0000050e e4 07           _mov       #0x7,skip_checksum_check
00000510 64 03           mov        status,skip_checksum_check
00000512 24 48           tst        skip_checksum_check,skip_checksum_check
00000514 8b 01           bf         LAB_0000051a
00000516 ee 00           mov        #0x0,r14
00000518 9d 38           mov.w      @(DAT_0000058c,pc),jump_address                  = 06C8h
                      LAB_0000051a                                    XREF[2]:     0000050a(j), 00000514(j)  
0000051a 60 e3           mov        r14,status
0000051c 88 01           cmp/eq     0x1,status
0000051e 8b 20           bf         jump_to_main
00000520 d4 20           mov.l      @(PTR_PTR_ROM_START_000005a4,pc),skip_checksum   = 0007fffc
00000522 9e 34           mov.w      @(DAT_0000058e,pc),r14                           = 1000h
00000524 60 42           mov.l      @skip_checksum_check=>->ROM_START,status         = 00002000
00000526 88 ff           cmp/eq     -0x1,status
00000528 89 03           bt         LAB_00000532
0000052a 60 42           mov.l      @skip_checksum_check=>->ROM_START,status         = 00002000
0000052c 60 02           mov.l      @status=>ROM_START,status                        = "60E1A500"
0000052e 88 ff           cmp/eq     -0x1,status
00000530 8b 09           bf         LAB_00000546
                      LAB_00000532                                    XREF[2]:     00000528(j), 00000540(j)  
00000532 b0 3d           bsr        FUN_000005b0                                     bool FUN_000005b0(ushort numLoops)
00000534 e4 07           _mov       #0x7,skip_checksum_check
00000536 64 03           mov        status,skip_checksum_check
00000538 24 48           tst        skip_checksum_check,skip_checksum_check
0000053a 89 11           bt         set_main
0000053c 60 e2           mov.l      @r14=>->FUN_000012b4,status                      = 000012b4
0000053e 88 ff           cmp/eq     -0x1,status
00000540 89 f7           bt         LAB_00000532
00000542 a0 03           bra        LAB_0000054c
00000544 00 09           _nop
                      LAB_00000546                                    XREF[1]:     00000530(j)  
00000546 60 e2           mov.l      @r14=>->FUN_000012b4,status                      = 000012b4
00000548 88 ff           cmp/eq     -0x1,status
0000054a 89 01           bt         LAB_00000550
                      LAB_0000054c                                    XREF[1]:     00000542(j)  
0000054c a0 09           bra        jump_to_main
0000054e 6d e2           _mov.l     @r14=>->FUN_000012b4,jump_address                = 000012b4
                      LAB_00000550                                    XREF[1]:     0000054a(j)  
00000550 d2 15           mov.l      @(PTR_PTR_FUN_000005a8,pc),r2                    = 0007fff8
00000552 a0 06           bra        jump_to_main
00000554 6d 22           _mov.l     @r2=>->FUN_0000d49c,jump_address                 = 0000d49c
                      LAB_00000556                                    XREF[1]:     00000500(j)  
00000556 61 f0           mov.b      @r15=>Stack[-0x8],r1
00000558 93 1a           mov.w      @(DAT_00000590,pc),r3                            = DFA8h
0000055a 92 1a           mov.w      @(DAT_00000592,pc),r2                            = 08F6h
0000055c 42 0b           jsr        @r2=>gpio_init                                   void gpio_init(void)
0000055e 23 10           _mov.b     r1,@r3=>BOOL_ffffdfa8                            = ??
                      set_main                                        XREF[1]:     0000053a(j)  
00000560 9d 14           mov.w      @(DAT_0000058c,pc),jump_address                  = 06C8h
                      jump_to_main                                    XREF[3]:     0000051e(j), 0000054c(j), 00000552(j)  
00000562 d3 0e           mov.l      @(DAT_0000059c,pc),r3                            = 5AA5A55Ah
00000564 d2 0e           mov.l      @(->CHECKSUM,pc),r2                              = ffffdffc
00000566 22 32           mov.l      r3,@r2=>CHECKSUM                                 = ??
00000568 93 14           mov.w      @(DAT_00000594,pc),r3                            = 0040h
0000056a 43 0b           jsr        @r3=>jump_to_main                                void jump_to_main(mainJump * add
0000056c 64 d3           _mov       jump_address=>FUN_000012b4,skip_checksum_check
                      -- Flow Override: CALL_RETURN (COMPUTED_CALL_TERMINATOR)
                      LAB_0000056e                                    XREF[1]:     0000056e(j)  
0000056e af fe           bra        LAB_0000056e
00000570 00 09           _nop
