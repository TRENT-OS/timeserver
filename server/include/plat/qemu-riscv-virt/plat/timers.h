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

#define HARDWARE_TIMER_INTERFACES                                                               \
    emits Dummy dummy_source;                                                                   \
    consumes Dummy qemu_goldfish_rtc;
#define HARDWARE_TIMER_ATTRIBUTES
#define HARDWARE_TIMER_COMPOSITION                                                              \
        connection seL4DTBHardware qemu_goldfish_rtc_conn(from dummy_source, to qemu_goldfish_rtc);
#define HARDWARE_TIMER_CONFIG                                                                   \
        qemu_goldfish_rtc.dtb = dtb({"path" : "/soc/rtc@101000"});                                \
        qemu_goldfish_rtc.generate_interrupts = 1;
#define HARDWARE_TIMER_PLAT_INTERFACES