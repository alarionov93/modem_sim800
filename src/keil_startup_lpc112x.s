;/*****************************************************************************
; * @file:    startup_LPC11xx.s
; * @purpose: CMSIS Cortex-M0 Core Device Startup File 
; *           for the NXP LPC11xx Device Series 
; * @version: V1.0
; * @date:    25. Nov. 2008
; *------- <<< Use Configuration Wizard in Context Menu >>> ------------------
; *
; * Copyright (C) 2008 ARM Limited. All rights reserved.
; * ARM Limited (ARM) is supplying this software for use with Cortex-M0 
; * processor based microcontrollers.  This file can be freely distributed 
; * within development tools that are supporting such ARM based processors. 
; *
; * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
; * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
; * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
; * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
; * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
; *
; *****************************************************************************/


; <h> Stack Configuration
;   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Stack_Size      EQU     0x00000200

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp


; <h> Heap Configuration
;   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Heap_Size       EQU     0x00000000

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit


                PRESERVE8
                THUMB


; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler
                DCD     NMI_Handler               ; NMI Handler
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     MemManage_Handler         ; MPU Fault Handler
                DCD     BusFault_Handler          ; Bus Fault Handler
                DCD     UsageFault_Handler        ; Usage Fault Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; SVCall Handler
                DCD     DebugMon_Handler          ; Debug Monitor Handler
                DCD     0                         ; Reserved
                DCD     PendSV_Handler            ; PendSV Handler
                DCD     SysTick_Handler           ; SysTick Handler

				; Wakeup sources for the I/O pins:
				;    PIO0 (0:11)
				;    PIO1 (0)
				DCD WAKEUP_IRQHandler ; PIO0_0  Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_1  Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_2  Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_3  Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_4  Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_5  Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_6  Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_7  Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_8  Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_9  Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_10 Wakeup
				DCD WAKEUP_IRQHandler ; PIO0_11 Wakeup
				DCD WAKEUP_IRQHandler ; PIO1_0  Wakeup
				DCD ADCB_IRQHandler ; ADC B sequence
				DCD SSP1_IRQHandler ; SPI/SSP1 Interrupt
				DCD I2C_IRQHandler ; I2C0
				DCD TIMER16_0_IRQHandler ; CT16B0 (16-bit Timer 0)
				DCD TIMER16_1_IRQHandler ; CT16B1 (16-bit Timer 1)
				DCD TIMER32_0_IRQHandler ; CT32B0 (32-bit Timer 0)
				DCD TIMER32_1_IRQHandler ; CT32B1 (32-bit Timer 1)
				DCD SSP0_IRQHandler ; SPI/SSP0 Interrupt
				DCD UART_IRQHandler ; UART0
				DCD UART1_IRQHandler ; UART1
				DCD UART2_IRQHandler ; UART2
				DCD ADCA_IRQHandler ; ADC A sequence (A/D Converter)
				DCD WDT_IRQHandler ; WDT   (Watchdog Timer)
				DCD BOD_IRQHandler ; BOD   (Brownout Detect)
				DCD Reserved_IRQHandler
				DCD PIOINT3_IRQHandler ; PIO INT3
				DCD PIOINT2_IRQHandler ; PIO INT2
				DCD PIOINT1_IRQHandler ; PIO INT1
				DCD PIOINT0_IRQHandler ; PIO INT0

                IF      :LNOT::DEF:NO_CRP
                AREA    |.ARM.__at_0x02FC|, CODE, READONLY
CRP_Key         DCD     0xFFFFFFFF
                ENDIF


                AREA    |.text|, CODE, READONLY


; Reset Handler

Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  SystemInit
                IMPORT  __main
                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP


; Dummy Exception Handlers (infinite loops which can be modified)                

; now, under COMMON NMI.c and NMI.h, a real NMI handler is created if NMI is enabled 
; for particular peripheral.
NMI_Handler     PROC
                EXPORT  NMI_Handler               [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler         [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler         [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler          [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler        [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler               [WEAK]
                B       .
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler          [WEAK]
                B       .
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler            [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler           [WEAK]
                B       .
                ENDP
Reserved_IRQHandler PROC
                EXPORT  Reserved_IRQHandler       [WEAK]
                B       .
                ENDP

Default_Handler PROC
				EXPORT WAKEUP_IRQHandler        [WEAK]
				EXPORT ADCB_IRQHandler          [WEAK]
				EXPORT SSP1_IRQHandler          [WEAK]
				EXPORT I2C_IRQHandler           [WEAK]
    			EXPORT TIMER16_0_IRQHandler     [WEAK]
				EXPORT TIMER16_1_IRQHandler     [WEAK]
				EXPORT TIMER32_0_IRQHandler     [WEAK]
				EXPORT TIMER32_1_IRQHandler     [WEAK]
				EXPORT SSP0_IRQHandler          [WEAK]
				EXPORT UART_IRQHandler          [WEAK]
				EXPORT UART1_IRQHandler         [WEAK]
				EXPORT UART2_IRQHandler         [WEAK]
				EXPORT ADCA_IRQHandler          [WEAK]
				EXPORT WDT_IRQHandler           [WEAK]
				EXPORT BOD_IRQHandler           [WEAK]
				EXPORT PIOINT3_IRQHandler       [WEAK]
				EXPORT PIOINT2_IRQHandler       [WEAK]
				EXPORT PIOINT1_IRQHandler       [WEAK]
				EXPORT PIOINT0_IRQHandler       [WEAK]
WAKEUP_IRQHandler
ADCB_IRQHandler
SSP1_IRQHandler
I2C_IRQHandler
TIMER16_0_IRQHandler
TIMER16_1_IRQHandler
TIMER32_0_IRQHandler
TIMER32_1_IRQHandler
SSP0_IRQHandler
UART_IRQHandler
UART1_IRQHandler
UART2_IRQHandler
ADCA_IRQHandler
WDT_IRQHandler
BOD_IRQHandler
PIOINT3_IRQHandler
PIOINT2_IRQHandler
PIOINT1_IRQHandler
PIOINT0_IRQHandler
                B       .

                ENDP

                ALIGN

; User Initial Stack & Heap

                IF      :DEF:__MICROLIB
                
                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit
                
                ELSE
                
                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap
__user_initial_stackheap

                LDR     R0, =  Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR

                ALIGN

                ENDIF

                END
