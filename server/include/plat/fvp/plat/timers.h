/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define HARDWARE_TIMER_INTERFACES \
    emits Dummy dummy_source; \
    consumes Dummy timer0; \
    consumes Dummy timer1;

/* no attributes */
#define HARDWARE_TIMER_ATTRIBUTES

#define HARDWARE_TIMER_COMPOSITION \
        connection seL4DTBHardware timer0_conn(from dummy_source, to timer0); \
        connection seL4DTBHardware timer1_conn(from dummy_source, to timer1);

#define HARDWARE_TIMER_CONFIG \
        timer0.dtb = dtb({"path" : "/timer@1c110000"}); \
        timer0.generate_interrupts = 1; \
        timer1.dtb = dtb({"path" : "/timer@1c120000"}); \
        timer1.generate_interrupts = 1;

/* no platform interfaces */
#define HARDWARE_TIMER_PLAT_INTERFACES
