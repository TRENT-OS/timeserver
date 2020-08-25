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

#include "plat.h"

/* ltimer for accessing timer devices */
static ltimer_t ltimer;
/* time manager for timeout multiplexing */
static time_manager_t timeManager;

/* declare the memory needed for the clients
 * this field tracks which timeouts have triggered
 * for a specific client */
uint32_t* client_state = NULL;

/* Prototype for this function is not generated by the camkes templates yet */
seL4_Word timeServer_rpc_get_sender_id();
void timeServer_rpc_emit(
    unsigned int);
int timeServer_rpc_largest_badge(
    void);

static ps_io_ops_t io_ops;

// Private Functions -----------------------------------------------------------

static inline uint64_t
currentTimeNs(
    void)
{
    uint64_t time;

    int error = ltimer_get_time(&ltimer, &time);
    ZF_LOGF_IF(error, "Failed to get time");

    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER, 7),
	      (time >> 32) & 0xffffffffU, time & 0xffffffffU, 0);

    return time;
}

static inline unsigned int
getTimeToken(
    int cid,
    int tid)
{
    return (unsigned int) cid * timers_per_client + tid;
}


static uint64_t _time(int cid);

static int
signalClient(
    uintptr_t token)
{
    int cid = ((int) token) / timers_per_client;
    int tid = ((int) token) % timers_per_client;
    uint64_t time =  _time(timeServer_rpc_get_sender_id() - 1);

    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER, 5),
        token, (time >> 32) & 0xffffffffU, time & 0xffffffffU);

    assert(client_state != NULL);

    client_state[cid] |= BIT(tid);
    timeServer_rpc_emit(cid + 1);

    return 0;
}

static void
timerHandler(
    UNUSED void*   emptyToken,
    ltimer_event_t lTimerEvent)
{
    int error = clientMux_lock();

    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER, 6), 0, 0, 0);
    ZF_LOGF_IF(error, "Failed to lock time server");

    error = tm_update(&timeManager);
    ZF_LOGF_IF(error, "Failed to update time manager");

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");
}

static int
_oneshot_relative(
    int      cid,
    int      tid,
    uint64_t ns)
{
    if (tid >= timers_per_client || tid < 0)
    {
        ZF_LOGE("invalid tid, 0 >= %d >= %d\n", tid, timers_per_client);
        return -1;
    }

    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    unsigned int id = getTimeToken(cid, tid);
    error = tm_register_rel_cb(&timeManager, ns, id, signalClient,
                               (uintptr_t) id);
    ZF_LOGF_IF(error, "Failed to set timeout");

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");
    return 0;
}

static int
_oneshot_absolute(
    int      cid,
    int      tid,
    uint64_t ns)
{
    if (tid >= timers_per_client || tid < 0)
    {
        ZF_LOGE("invalid tid, 0 >= %d >= %d\n", tid, timers_per_client);
        return -1;
    }

    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    unsigned int token = getTimeToken(cid, tid);

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
    return 0;
}

static int
_periodic(
    int      cid,
    int      tid,
    uint64_t ns)
{
    if (tid >= timers_per_client || tid < 0)
    {
        ZF_LOGE("invalid tid, 0 >= %d >= %d\n", tid, timers_per_client);
        return -1;
    }

    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    unsigned int token = getTimeToken(cid, tid);
    error = tm_register_periodic_cb(&timeManager, ns, 0, token, signalClient,
                                    (uintptr_t) token);
    ZF_LOGF_IF(error, "Failed to set timeout");

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");
    return 0;
}

static int
_stop(
    int cid,
    int tid)
{
    if (tid >= timers_per_client || tid < 0)
    {
        ZF_LOGE("invalid tid, 0 >= %d >= %d\n", tid, timers_per_client);
        return -1;
    }
    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    error = tm_deregister_cb(&timeManager, getTimeToken(cid, tid));
    ZF_LOGF_IF(error, "Failed to deregister callback");

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");
    return 0;
}

static unsigned int
_completed(
    int cid)
{
    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock time server");

    assert(client_state != NULL);
    unsigned int ret = client_state[cid];
    client_state[cid] = 0;

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock time server");

    return ret;
}

static uint64_t
_time(
    int cid)
{
    return currentTimeNs();
}

// Public Functions -----------------------------------------------------------

/* substract 1 from the badge as we started counting badges at 1
 * to avoid using the 0 badge */
int timeServer_rpc_oneshot_relative(
    int      id,
    uint64_t ns)
{

    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER_RPC, 0),
	      getTimeToken(timeServer_rpc_get_sender_id() - 1, id), (ns >> 32) & 0xffffffffU, ns & 0xffffffffU);
    return _oneshot_relative(timeServer_rpc_get_sender_id() - 1, id, ns);
}

int
timeServer_rpc_oneshot_absolute(
    int      id,
    uint64_t ns)
{
    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER_RPC, 1),
	      getTimeToken(timeServer_rpc_get_sender_id() - 1, id),
        (ns >> 32) & 0xffffffffU, ns & 0xffffffffU);
    return _oneshot_absolute(timeServer_rpc_get_sender_id() - 1, id, ns);
}


int
timeServer_rpc_periodic(
    int      id,
    uint64_t ns)
{
    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER_RPC, 2),
	      getTimeToken(timeServer_rpc_get_sender_id() - 1, id),
        (ns >> 32) & 0xffffffffU, ns & 0xffffffffU);

    return _periodic(timeServer_rpc_get_sender_id() - 1, id, ns);
}

int
timeServer_rpc_stop(
    int id)
{
    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER_RPC, 5),
	       getTimeToken(timeServer_rpc_get_sender_id() - 1, id),
         0, 0);

    return _stop(timeServer_rpc_get_sender_id() - 1, id);
}

unsigned int
timeServer_rpc_completed(
    void)
{
    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER_RPC, 4),
        timeServer_rpc_get_sender_id() - 1,
        0, 0);

    return _completed(timeServer_rpc_get_sender_id() - 1);
}

uint64_t
timeServer_rpc_time(
    void)
{
    uint64_t time =  _time(timeServer_rpc_get_sender_id() - 1);

    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER_RPC, 3),
	      timeServer_rpc_get_sender_id() - 1,
        (time >> 32) & 0xffffffffU, time & 0xffffffffU);

    return time;
  // return _time(timeServer_rpc_get_sender_id() - 1);
}

void
post_init(
    void)
{
   log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER, 0),
	     0, 0, 0);

    int error = clientMux_lock();
    ZF_LOGF_IF(error, "Failed to lock timer server");

    error = camkes_io_ops(&io_ops);
    ZF_LOGF_IF(error, "Failed to get camkes_io_ops");

    error = ps_calloc(&(io_ops.malloc_ops), timeServer_rpc_largest_badge(),
                      sizeof(*client_state), (void**) &client_state);
    ZF_LOGF_IF(error, "Failed to allocate client state");

    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER, 1),
        0, 0, 0);

    if (plat_pre_init)
    {
        plat_pre_init();
    }
    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER, 2),
        0, 0, 0);

    error = ltimer_default_init(&ltimer, io_ops, timerHandler, NULL);
    ZF_LOGF_IF(error, "Failed to init timer");

    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER, 3),
        0, 0, 0);

    if (plat_post_init)
    {
        plat_post_init(&ltimer);
    }

    log32v(LOG_ID(LOG_MAJ_TIMESERVER,LOG_SUB_TIMERSERVER, 4),
        0, 0, 0);

    int num_timers = timers_per_client * timeServer_rpc_largest_badge();
    tm_init(&timeManager, &ltimer, &io_ops, num_timers);
    for (unsigned int i = 0; i < num_timers; i++)
    {
        error = tm_alloc_id_at(&timeManager, i);
        ZF_LOGF_IF(error, "Failed to alloc id at %u\n", i);
    }

    error = clientMux_unlock();
    ZF_LOGF_IF(error, "Failed to unlock timer server");
}