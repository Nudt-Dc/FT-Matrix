.global vsip_vindexbool

vsip_vindexbool:

.preparation:
; cycle(6)
        SMVAGA36.M1     R11:R10, AR0
|       SMOVIL          0, R1
        SMOVIH          0, R1
        SMVAGA36.M1     R13:R12, AR10
|       SMOVIL          16, R2
        SADD.M2         0, R1, R3
|       SMOVIH          0, R2
        SMVAGA36.M1     R3:R2, OR0
        SADD.M2         0, R1, R8

.boolloop:
; cycle(9)
        SSUB.M2         R1, R14, R9
        SLT             16, R9, R0
[R0]    SBR             .A1
        SNOP            6

.boolloop2:
; cycle(2)
        SMVCCG          SVR0, R15
        SNOP            1

; cycle(8)
        SEQ             0, R15, R0
[R0]    SBR             .L16
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L16:
; cycle(9)
        SADD.M2         1, R1, R1
        SLT             R1, R14, R0
[R0]    SBR             .boolloop2
        SNOP            6

; cycle(7)
        SBR             .A2
        SNOP            6

.A1:
; cycle(11)
        VLDW            *AR0++[OR0], VR0
        SNOP            7
        VMVCGC          VR0, SVR
        SNOP            2

; cycle(2)
        SMVCCG          SVR0, R16
        SNOP            1

; cycle(8)
        SEQ             0, R16, R0
[R0]    SBR             .L0
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L0:
; cycle(2)
        SMVCCG          SVR1, R17
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R17, R0
[R0]    SBR             .L1
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L1:
; cycle(2)
        SMVCCG          SVR2, R18
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R18, R0
[R0]    SBR             .L2
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L2:
; cycle(2)
        SMVCCG          SVR3, R19
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R19, R0
[R0]    SBR             .L3
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L3:
; cycle(2)
        SMVCCG          SVR4, R20
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R20, R0
[R0]    SBR             .L4
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L4:
; cycle(2)
        SMVCCG          SVR5, R21
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R21, R0
[R0]    SBR             .L5
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L5:
; cycle(2)
        SMVCCG          SVR6, R22
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R22, R0
[R0]    SBR             .L6
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L6:
; cycle(2)
        SMVCCG          SVR7, R23
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R23, R0
[R0]    SBR             .L7
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L7:
; cycle(2)
        SMVCCG          SVR8, R24
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R24, R0
[R0]    SBR             .L8
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L8:
; cycle(2)
        SMVCCG          SVR9, R25
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R25, R0
[R0]    SBR             .L9
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L9:
; cycle(2)
        SMVCCG          SVR10, R26
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R26, R0
[R0]    SBR             .L10
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L10:
; cycle(2)
        SMVCCG          SVR11, R27
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R27, R0
[R0]    SBR             .L11
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L11:
; cycle(2)
        SMVCCG          SVR12, R28
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R28, R0
[R0]    SBR             .L12
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L12:
; cycle(2)
        SMVCCG          SVR13, R29
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R29, R0
[R0]    SBR             .L13
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L13:
; cycle(2)
        SMVCCG          SVR14, R42
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R42, R0
[R0]    SBR             .L14
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L14:
; cycle(2)
        SMVCCG          SVR15, R43
        SADD.M2         1, R1, R1

; cycle(8)
        SEQ             0, R43, R0
[R0]    SBR             .L15
        SNOP            6

; cycle(1)
        SADD.M2         1, R8, R8
|       SSTW            R1, *AR10++[1]

.L15:
; cycle(1)
        SADD.M2         1, R1, R1

.A2:
; cycle(8)
        SLT             R1, R14, R0
[R0]    SBR             .boolloop
        SNOP            6

.restoration:
; cycle(7)
        SMOV            R8, R10                         ;transfer "retval" to R10
|       SBR             R62
        SNOP            6

.size vsip_vindexbool, .-vsip_vindexbool