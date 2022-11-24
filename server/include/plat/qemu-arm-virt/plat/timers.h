/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* no need to define a HARDWARE_TIMER_COMPONENT */

#define HARDWARE_TIMER_INTERFACES \
    emits Dummy dummy_source; \
    consumes Dummy timer_sp804;

/* no attributes */
#define HARDWARE_TIMER_ATTRIBUTES

#define HARDWARE_TIMER_COMPOSITION \
    connection seL4DTBHardware timer_conn(from dummy_source, to timer_sp804);

#define HARDWARE_TIMER_CONFIG \
    timer_sp804.dtb = dtb({"path" : "/sp804@b000000"}); \
    timer_sp804.generate_interrupts = 1;

/* no platform interfaces */
#define HARDWARE_TIMER_PLAT_INTERFACES
