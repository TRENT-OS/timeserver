/* Copyright (C) 2020, Hensoldt Cyber GmbH */

#include "OS_Error.h"

#include "TimeServer.h"

#include <stdint.h>

void
TimeServer_sleep(
    const if_OS_Timer_t*         rpc,
    const TimeServer_Precision_t prec,
    const uint64_t               val)
{
    uint64_t ns;

    if (NULL == rpc)
    {
        return;
    }

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

    if (rpc->oneshot_relative(0, ns) == OS_SUCCESS)
    {
        rpc->notify_wait();
    }
}

uint64_t
TimeServer_getTime(
    const if_OS_Timer_t*         rpc,
    const TimeServer_Precision_t prec)
{
    uint64_t ns;

    if (NULL == rpc)
    {
        return 0;
    }

    if (rpc->time(&ns) == OS_SUCCESS)
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