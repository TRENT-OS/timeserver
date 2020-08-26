/* Copyright (C) 2020, Hensoldt Cyber GmbH */

#include "OS_Error.h"

#include "TimeServer.h"

#include <stdint.h>

OS_Error_t
TimeServer_sleep(
    const if_OS_Timer_t*         rpc,
    const TimeServer_Precision_t prec,
    const uint64_t               val)
{
    OS_Error_t err;
    uint64_t ns;

    if (NULL == rpc)
    {
        return OS_ERROR_INVALID_PARAMETER;
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
        return OS_ERROR_INVALID_PARAMETER;
    }

    if ((err = rpc->oneshot_relative(0, ns)) != OS_SUCCESS)
    {
        return err;
    }

    rpc->notify_wait();

    return OS_SUCCESS;
}

OS_Error_t
TimeServer_getTime(
    const if_OS_Timer_t*         rpc,
    const TimeServer_Precision_t prec,
    uint64_t*                    val)
{
    OS_Error_t err;
    uint64_t ns;

    if (NULL == rpc)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    if ((err = rpc->time(&ns)) != OS_SUCCESS)
    {
        return err;
    }

    switch (prec)
    {
    case TimeServer_PRECISION_SEC:
        *val = ns / NS_IN_S;
        break;
    case TimeServer_PRECISION_MSEC:
        *val = ns / NS_IN_MS;
        break;
    case TimeServer_PRECISION_USEC:
        *val = ns / NS_IN_US;
        break;
    case TimeServer_PRECISION_NSEC:
        *val = ns;
        break;
    default:
        return OS_ERROR_INVALID_PARAMETER;
    }

    return OS_SUCCESS;
}