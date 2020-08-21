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
 * @param prec (required) precision of timer value
 * @param val (required) time to sleep
 */
void
TimeServer_sleep(
    const TimeServer_Precision_t prec,
    const uint64_t               val);

/**
 * @brief Get the current system time
 *
 * @param prec (required) precision of current time
 *
 * @return Timestamp in selected precision
 */
uint64_t
TimeServer_getTime(
    const TimeServer_Precision_t prec);

/** @} */