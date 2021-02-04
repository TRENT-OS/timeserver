/*
 * Copyright (C) 2020-2021, HENSOLDT Cyber GmbH
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @defgroup TimeServer component
 * @{
 *
 * @file
 *
 * @brief TimeServer convenience functions to hide CAmkES details
 */

#pragma once

#include "interfaces/if_OS_Timer.h"

#include <stdint.h>

typedef enum
{
    TimeServer_PRECISION_SEC = 0,
    TimeServer_PRECISION_MSEC,
    TimeServer_PRECISION_USEC,
    TimeServer_PRECISION_NSEC,
} TimeServer_Precision_t;

/**
 * @brief Sleep for some time
 *
 * @param rpc (required) pointer to if_OS_Timer rpc struct
 * @param prec (required) precision of timer value
 * @param val (required) time to sleep
 *
 * @return an error code
 * @retval OS_SUCCESS if operation succeeded
 * @retval OS_ERROR_INVALID_PARAMETER if a parameter was missing or invalid
 * @retval OS_ERROR_NOT_FOUND if TimeServer could not find corresponding
 *  client state
  */
OS_Error_t
TimeServer_sleep(
    const if_OS_Timer_t*         rpc,
    const TimeServer_Precision_t prec,
    const uint64_t               val);

/**
 * @brief Get the current system time
 *
 * @param rpc (required) pointer to if_OS_Timer rpc struct
 * @param prec (required) precision of current time
 * @param val (required) pointer to time value
 *
 * @return an error code
 * @retval OS_SUCCESS if operation succeeded
 * @retval OS_ERROR_INVALID_PARAMETER if a parameter was missing or invalid
 * @retval OS_ERROR_NOT_FOUND if TimeServer could not find corresponding
 *  client state
 */
OS_Error_t
TimeServer_getTime(
    const if_OS_Timer_t*         rpc,
    const TimeServer_Precision_t prec,
    uint64_t*                    val);

/** @} */
