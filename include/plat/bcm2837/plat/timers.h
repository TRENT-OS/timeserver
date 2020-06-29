
/**
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#pragma once

#define HARDWARE_TIMER_COMPONENT                                                \
    component HWTimer {                                                         \
        hardware;                                                               \
        dataport Buf(4096) mem;                                                 \
        emits HWTimerIRQ irq;                                                   \
    }                                                                           \

#define HARDWARE_TIMER_INTERFACES                                               \
    dataport Buf(4096) Sp804Timer_mem;                                          \
    dataport Buf(4096) SysTimer_mem;                                            \
    consumes HWTimerIRQ Sp804Timer_irq;                                         \
    consumes HWTimerIRQ SysTimer_irq;
#define HARDWARE_TIMER_ATTRIBUTES
#define HARDWARE_TIMER_COMPOSITION                                              \
        component HWTimer Sp804Timer;                                           \
        component HWTimer SysTimer;                                             \
        connection seL4HardwareMMIO Sp804Timer_mem(from Sp804Timer_mem,         \
                                                 to Sp804Timer.mem);            \
        connection seL4HardwareMMIO SysTimer_mem(from SysTimer_mem,             \
                                                 to SysTimer.mem);              \
        connection seL4HardwareInterrupt Sp804Timer_irq(from Sp804Timer.irq,    \
                                                      to Sp804Timer_irq);       \
        connection seL4HardwareInterrupt SysTimer_irq(from SysTimer.irq,        \
                                                      to SysTimer_irq);
#define HARDWARE_TIMER_CONFIG                                                   \
    Sp804Timer.mem_paddr        = 0x3f00b000;                                   \
    Sp804Timer.mem_size         = 0x1000;                                       \
    Sp804Timer.irq_irq_number   = 32;                                           \
    SysTimer.mem_paddr          = 0x3f003000;                                   \
    SysTimer.mem_size           = 0x1000;                                       \
    SysTimer.irq_irq_number     = 65;
#define HARDWARE_TIMER_PLAT_INTERFACES
