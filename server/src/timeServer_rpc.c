/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/*
 *  TimeServer
 *
 *  Copyright (C) 2020, Hensoldt Cyber GmbH
 */

#include <autoconf.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <sel4/sel4.h>
#include <sel4/arch/constants.h>
#include <camkes.h>
#include <camkes/irq.h>
#include <platsupport/time_manager.h>
#include <platsupport/local_time_manager.h>
#include <platsupport/irq.h>
#include <utils/util.h>
#include <sel4utils/sel4_zf_logif.h>
#include <simple/simple.h>
#include <camkes/io.h>

#include "OS_Error.h"
#include "LibDebug/Debug.h"

#include "plat.h"

#ifdef CONFIG_PLAT_ZYNQ7000
    /* Please be aware that tests surfaced that the timer implementation in QEMU
     * results in timings that deviate largely from the observed real time.
     * Multiplying the requested ns with the following factor helps to improve the
     * observed timer accuracy and since ZYNQ7000 is currently only supported in
     * QEMU, it is valid to use this build target as a filter.
     * The conversion factor is solely based on empirical tests done with varying
     * conversion factors. */
    #define FEATURE_CONVERSION_FACTOR
    #define APPLY_NS_CONVERSION(_ns_)  ( ( (_ns_) / 100) * 3 )
#endif

/* ltimer for accessing timer devices */
static ltimer_t ltimer;
/* time manager for timeout multiplexing */
static time_manager_t timeManager;

typedef struct
{
    unsigned int id;
    uint32_t timers;
    void (*notify)(void);
} TimeServer_Client;

// Maximum amount of clients allowed
#define TIMESERVER_CLIENTS_MAX 8
// Assign the clients
static TimeServer_Client timerClients[TIMESERVER_CLIENTS_MAX] = {
    { 1, 0, timeServer_notify1_emit },
    { 2, 0, timeServer_notify2_emit },
    { 3, 0, timeServer_notify3_emit },
    { 4, 0, timeServer_notify4_emit },
    { 5, 0, timeServer_notify5_emit },
    { 6, 0, timeServer_notify6_emit },
    { 7, 0, timeServer_notify7_emit },
};

/* Prototype for this function is not generated by the camkes templates yet */
seL4_Word timeServer_rpc_get_sender_id();

static ps_io_ops_t io_ops;

// Private Functions -----------------------------------------------------------

static inline uint64_t
currentTimeNs(
    void)
{
    uint64_t time;

    int error = ltimer_get_time(&ltimer, &time);
    ZF_LOGF_IF(error, "Failed to get time");

    return time;
}

static TimeServer_Client*
getClient(
    seL4_Word sender_id)
{
    TimeServer_Client* client;

    // Get client belonging to a sender_id; subtract 1 because we want to
    // avoid using the 0 badge...
    client = (sender_id > TIMESERVER_CLIENTS_MAX) || (sender_id <= 0) ? NULL :
             (timerClients[sender_id - 1].id != sender_id) ? NULL :
             &timerClients[sender_id - 1];

    return client;
}

static inline unsigned int
getTimeToken(
    TimeServer_Client* client,
    int                tid)
{
    return (unsigned int) client->id * timers_per_client + tid;
}

static int
signalClient(
    uintptr_t token)
{
    TimeServer_Client* client;

    int cid = ((int) token) / timers_per_client;
    int tid = ((int) token) % timers_per_client;

    client = &timerClients[cid - 1];
    client->timers |= BIT(tid);
    client->notify();

    return 0;
}

static void
timerHandler(
    UNUSED void*   emptyToken,
    ltimer_event_t lTimerEvent)
{
    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    error = tm_update(&timeManager);
    ZF_LOGF_IF(error, "Failed to update time manager");

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");
}

static OS_Error_t
_oneshot_relative(
    TimeServer_Client* client,
    int                tid,
    uint64_t           ns)
{
    if (tid >= timers_per_client || tid < 0)
    {
        ZF_LOGE("invalid tid, 0 >= %d >= %d\n", tid, timers_per_client);
        return OS_ERROR_INVALID_PARAMETER;
    }

    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    #ifdef FEATURE_CONVERSION_FACTOR
        ns = APPLY_NS_CONVERSION(ns);
    #endif

    unsigned int id = getTimeToken(client, tid);
    error = tm_register_rel_cb(&timeManager, ns, id, signalClient,
                               (uintptr_t) id);
    ZF_LOGF_IF(error, "Failed to set timeout");

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");

    return OS_SUCCESS;
}

static OS_Error_t
_oneshot_absolute(
    TimeServer_Client* client,
    int                tid,
    uint64_t           ns)
{
    if (tid >= timers_per_client || tid < 0)
    {
        ZF_LOGE("invalid tid, 0 >= %d >= %d\n", tid, timers_per_client);
        return OS_ERROR_INVALID_PARAMETER;
    }

    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    #ifdef FEATURE_CONVERSION_FACTOR
        ns = APPLY_NS_CONVERSION(ns);
    #endif

    unsigned int token = getTimeToken(client, tid);

    error = tm_register_abs_cb(&timeManager, ns, token, signalClient,
                               (uintptr_t) token);
    if (error == ETIME)
    {
        signalClient(token);
        error = 0;
    }
    ZF_LOGF_IF(error, "Failed to set timeout");

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");

    return OS_SUCCESS;
}

static OS_Error_t
_periodic(
    TimeServer_Client* client,
    int                tid,
    uint64_t           ns)
{
    if (tid >= timers_per_client || tid < 0)
    {
        ZF_LOGE("invalid tid, 0 >= %d >= %d\n", tid, timers_per_client);
        return OS_ERROR_INVALID_PARAMETER;
    }

    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    #ifdef FEATURE_CONVERSION_FACTOR
        ns = APPLY_NS_CONVERSION(ns);
        /* Following the underlying implementation, the ns passed need to be larger
         * than zero, otherwise it will not result in a periodically triggered
         * event */
        ns = (ns > 0) ? ns : 1;
    #endif

    unsigned int token = getTimeToken(client, tid);
    error = tm_register_periodic_cb(&timeManager, ns, 0, token, signalClient,
                                    (uintptr_t) token);
    ZF_LOGF_IF(error, "Failed to set timeout");

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");

    return OS_SUCCESS;
}

static OS_Error_t
_stop(
    TimeServer_Client* client,
    int                tid)
{
    if (tid >= timers_per_client || tid < 0)
    {
        ZF_LOGE("invalid tid, 0 >= %d >= %d\n", tid, timers_per_client);
        return OS_ERROR_INVALID_PARAMETER;
    }

    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    error = tm_deregister_cb(&timeManager, getTimeToken(client, tid));
    ZF_LOGF_IF(error, "Failed to deregister callback");

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");

    return OS_SUCCESS;
}

static OS_Error_t
_completed(
    TimeServer_Client* client,
    uint32_t*          tmr)
{
    if (NULL == tmr)
    {
        return OS_ERROR_INVALID_PARAMETER;
    }

    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    *tmr = client->timers;
    client->timers = 0;

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");

    return OS_SUCCESS;
}

static OS_Error_t
_time(
    TimeServer_Client* client,
    uint64_t*          ns)
{
    *ns  = currentTimeNs();

    return OS_SUCCESS;
}

// Public Functions -----------------------------------------------------------

#define GET_CLIENT(cli, cid)                                                        \
    if ((cli = getClient(cid)) == NULL) {                                           \
        Debug_LOG_ERROR("Could not get corresponding client state (cid=%i)", cid);  \
        return OS_ERROR_NOT_FOUND;                                                  \
    }

OS_Error_t
timeServer_rpc_oneshot_relative(
    int      id,
    uint64_t ns)
{
    TimeServer_Client* client;

    GET_CLIENT(client, timeServer_rpc_get_sender_id());

    return _oneshot_relative(client, id, ns);
}

OS_Error_t
timeServer_rpc_oneshot_absolute(
    int      id,
    uint64_t ns)
{
    TimeServer_Client* client;

    GET_CLIENT(client, timeServer_rpc_get_sender_id());

    return _oneshot_absolute(client, id, ns);
}

OS_Error_t
timeServer_rpc_periodic(
    int      id,
    uint64_t ns)
{
    TimeServer_Client* client;

    GET_CLIENT(client, timeServer_rpc_get_sender_id());

    return _periodic(client, id, ns);
}

OS_Error_t
timeServer_rpc_stop(
    int id)
{
    TimeServer_Client* client;

    GET_CLIENT(client, timeServer_rpc_get_sender_id());

    return _stop(client, id);
}

OS_Error_t
timeServer_rpc_completed(
    uint32_t* tmr)
{
    TimeServer_Client* client;

    GET_CLIENT(client, timeServer_rpc_get_sender_id());

    return _completed(client, tmr);
}

OS_Error_t
timeServer_rpc_time(
    uint64_t* ns)
{
    TimeServer_Client* client;

    GET_CLIENT(client, timeServer_rpc_get_sender_id());

    return _time(client, ns);
}

void
post_init(
    void)
{
    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock timer server");

    error = camkes_io_ops(&io_ops);
    ZF_LOGF_IF(error, "Failed to get camkes_io_ops");

    if (plat_pre_init)
    {
        plat_pre_init();
    }

    error = ltimer_default_init(&ltimer, io_ops, timerHandler, NULL);
    ZF_LOGF_IF(error, "Failed to init timer");

    if (plat_post_init)
    {
        plat_post_init(&ltimer);
    }

    int num_timers = timers_per_client * TIMESERVER_CLIENTS_MAX;
    tm_init(&timeManager, &ltimer, &io_ops, num_timers);
    for (unsigned int i = 0; i < num_timers; i++)
    {
        error = tm_alloc_id_at(&timeManager, i);
        ZF_LOGF_IF(error, "Failed to alloc id at %u\n", i);
    }

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock timer server");
}
