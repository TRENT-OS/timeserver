/*
 * Copyright (C) 2023, HENSOLDT Cyber GmbH
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#pragma once

 #define HARDWARE_TIMER_INTERFACES                                                   \
    emits Dummy dummy_source;                                                       \
    consumes Dummy timer0;                                                            \
    consumes Dummy timer1;
#define HARDWARE_TIMER_ATTRIBUTES
#define HARDWARE_TIMER_COMPOSITION                                                      \
        connection seL4DTBHardware timer0_conn(from dummy_source, to timer0);               \
        connection seL4DTBHardware timer1_conn(from dummy_source, to timer1);
#define HARDWARE_TIMER_CONFIG                                                       \
        timer0.dtb = dtb({"path" : "/sp804@90c0000"});                          \
        timer0.generate_interrupts = 1;                                               \
        timer1.dtb = dtb({"path" : "/sp804@90d0000"});                          \
        timer1.generate_interrupts = 1;
#define HARDWARE_TIMER_PLAT_INTERFACES
