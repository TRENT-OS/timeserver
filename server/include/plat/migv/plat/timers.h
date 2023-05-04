/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#pragma once

#define HARDWARE_TIMER_INTERFACES                                                               \
    emits Dummy dummy_source;                                                                   \
    consumes Dummy migv_timer0;                                                                 \
    consumes Dummy migv_timer1;
#define HARDWARE_TIMER_ATTRIBUTES
#define HARDWARE_TIMER_COMPOSITION                                                              \
        connection seL4DTBHardware migv_timer0_conn(from dummy_source, to migv_timer0);         \
        connection seL4DTBHardware migv_timer1_conn(from dummy_source, to migv_timer1);
#define HARDWARE_TIMER_CONFIG                                                                   \
        migv_timer0.dtb = dtb({"path" : "/soc/timer@00409000"});                                \
        migv_timer0.generate_interrupts = 1;                                                    \
        migv_timer1.dtb = dtb({"path" : "/soc/timer@0040a000"});                                \
        migv_timer1.generate_interrupts = 1;
#define HARDWARE_TIMER_PLAT_INTERFACES
