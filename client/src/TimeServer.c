/* Copyright (C) 2020, Hensoldt Cyber GmbH */

#include "OS_Error.h"

#include "TimeServer.h"

#include <stdint.h>

#include <camkes.h>

// CAmkES does not generate this ... yet...
seL4_CPtr timeServer_rpc_notification(
    void);

void
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

    if (timeServer_rpc_oneshot_relative(0, ns) == OS_SUCCESS)
    {
        seL4_Wait(notification, &badge);
    }
}

uint64_t
TimeServer_getTime(
    const TimeServer_Precision_t prec)
{
    uint64_t ns;

    if (timeServer_rpc_time(&ns) == OS_SUCCESS)
    {
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
    }

    return 0;
}