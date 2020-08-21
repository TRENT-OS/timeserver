/**
 * Copyright (C) 2020, Hensoldt Cyber GmbH
 *
 * @defgroup TimeServer component
 * @{
 *
 * @file
 *
 * @brief TimeServer convenience functions to hide CAmkES details
 *
 */
#pragma once

#include <stdint.h>

#include <camkes.h>

typedef enum
{
    TimeServer_PRECISION_SEC = 0,
    TimeServer_PRECISION_MSEC,
    TimeServer_PRECISION_USEC,
    TimeServer_PRECISION_NSEC,
} TimeServer_Precision_t;

// CAmkES does not generate this ... yet...
seL4_CPtr timeServer_rpc_notification(
    void);

/**
 * @brief Sleep for some time
 *
 * @param prec (required) precision of timer value
 * @param val (required) time to sleep
 */
__attribute__((unused))
static inline void
TimeServer_sleep(
    const TimeServer_Precision_t prec,
    const uint64_t               val)
{
    uint64_t ns;
    seL4_Word badge;
    seL4_CPtr notification = timeServer_rpc_notification();

    switch (prec)
    {
    case TimeServer_PRECISION_SEC:
        ns = val * NS_IN_S;
        break;
    case TimeServer_PRECISION_MSEC:
        ns = val * NS_IN_MS;
        break;
    case TimeServer_PRECISION_USEC:
        ns = val * NS_IN_US;
        break;
    case TimeServer_PRECISION_NSEC:
        ns = val;
        break;
    default:
        return;
    }

    timeServer_rpc_oneshot_relative(0, ns);
    seL4_Wait(notification, &badge);
}

/**
 * @brief Get the current system time
 *
 * @param prec (required) precision of current time
 *
 * @return Timestamp in selected precision
 */
__attribute__((unused))
static inline uint64_t
TimeServer_getTime(
    const TimeServer_Precision_t prec)
{
    uint64_t ns = timeServer_rpc_time();

    switch (prec)
    {
    case TimeServer_PRECISION_SEC:
        return ns / NS_IN_S;
    case TimeServer_PRECISION_MSEC:
        return ns / NS_IN_MS;
    case TimeServer_PRECISION_USEC:
        return ns / NS_IN_US;
    case TimeServer_PRECISION_NSEC:
        return ns;
    default:
        break;
    }

    return 0;
}

/** @} */