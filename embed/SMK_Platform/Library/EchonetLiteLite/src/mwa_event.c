/*****************************************************************************/
/* ECHONET Lite Lite                                                         */
/*                                                                           */
/* (c) Copyright 2014, SMK Corporation. All rights reserved.                 */
/*****************************************************************************/

#include "ell_adapter.h"

//*****************************************************************************
// Event and Event Queue
//*****************************************************************************

//=============================================================================
bool_t MWA_InitEventQueue(MWA_EventQueue_t *queue)
{
    if (queue == NULL) return (FALSE);

    MEM_Set(queue, 0, sizeof(MWA_EventQueue_t));

#ifdef __FREESCALE_MQX__
    _lwsem_create(&queue->sem, 1);
#endif

    return (TRUE);
}

//=============================================================================
bool_t MWA_AddEventToQueue(MWA_EventQueue_t *queue, MWA_Event_t *event)
{
    if (queue == NULL || event == NULL) return (FALSE);
    if (queue->size >= MWA_EVENT_QUEUE_SIZE) return (FALSE);

#ifdef __FREESCALE_MQX__
    _lwsem_wait(&queue->sem);
#endif

    queue->queue[queue->w_ptr] = event;
    queue->w_ptr ++;
    if (queue->w_ptr >= MWA_EVENT_QUEUE_SIZE) {
        queue->w_ptr = 0;
    }
    queue->size ++;

#ifdef __FREESCALE_MQX__
    _lwsem_post(&queue->sem);
#endif

    return (TRUE);
}

//=============================================================================
MWA_Event_t *MWA_GetEventFromQueue(MWA_EventQueue_t *queue)
{
    MWA_Event_t *event;

    if (queue == NULL) return (NULL);
    if (queue->size == 0) return (NULL);

#ifdef __FREESCALE_MQX__
    _lwsem_wait(&queue->sem);
#endif

    event = queue->queue[queue->r_ptr];
    queue->r_ptr ++;
    if (queue->r_ptr >= MWA_EVENT_QUEUE_SIZE) {
        queue->r_ptr = 0;
    }
    queue->size --;

#ifdef __FREESCALE_MQX__
    _lwsem_post(&queue->sem);
#endif

    return (event);
}

//=============================================================================
bool_t MWA_IsQueueFull(MWA_EventQueue_t *queue)
{
    if (queue == NULL) return (TRUE);

    return ((queue->size == MWA_EVENT_QUEUE_SIZE));
}

//=============================================================================
MWA_Event_t *MWA_AllocEvent(uint8_t id, int payload_len)
{
    MWA_Event_t *event;

    event = MEM_Alloc(sizeof(MWA_Event_t) + payload_len);
    if (event == NULL) return (NULL);

    MEM_Set(event, 0, sizeof(MWA_Event_t) + payload_len);

    event->id = id;
    event->len = payload_len;

    return (event);
}

//=============================================================================
void MWA_FreeEvent(MWA_Event_t *event)
{
    if (event == NULL) return;

    MEM_Free(event);
}

//*****************************************************************************
// Timer
//*****************************************************************************

//=============================================================================
bool_t MWA_InitTimer(MWA_TimerHandle_t *timer,
                     MWA_TimerCallback_t *func, void *data)
{
    if (timer == NULL) return (FALSE);

    timer->func = func;
    timer->data = data;
    timer->tag = 0;

#ifndef __FREESCALE_MQX__
    timer->cur_time = 0;
    timer->next_time = 0;
#else
    timer->id = TIMER_NULL_ID;
#endif

    return (TRUE);
}

//=============================================================================
#ifdef __FREESCALE_MQX__
static void MWA_TimerCallback(_timer_id id, void *data,
                              uint32_t sec, uint32_t msec)
{
    MWA_TimerHandle_t *timer = (MWA_TimerHandle_t *)data;

    if (timer == NULL) return;

    MWA_InvokeTimerCallback(timer);
}
#endif

//=============================================================================
void MWA_StartTimer(MWA_TimerHandle_t *timer, int milli_sec, uint32_t tag)
{
    if (timer == NULL) return;

    timer->tag = tag;

#ifndef __FREESCALE_MQX__
    timer->cur_time = MWA_GetMilliSec();
    timer->next_time = timer->cur_time + milli_sec;
#else
    timer->id = _timer_start_oneshot_after(MWA_TimerCallback, timer,
                                           TIMER_ELAPSED_TIME_MODE, milli_sec);
#endif
}

//=============================================================================
void MWA_CancelTimer(MWA_TimerHandle_t *timer)
{
    if (timer == NULL) return;

    if (timer->func != NULL) {
        (*(timer->func))(timer->data, timer->tag, TRUE);
    }

#ifndef __FREESCALE_MQX__
    timer->cur_time = 0;
    timer->next_time = 0;
#else
    if (timer->id != TIMER_NULL_ID) {
        _timer_cancel(timer->id);
        timer->id = TIMER_NULL_ID;
    }
#endif
}

//=============================================================================
#ifndef __FREESCALE_MQX__
uint32_t MWA_GetNextTime(MWA_TimerHandle_t *timer)
{
    if (timer == NULL) return (0);

    if (timer->cur_time == 0) return (0);
    if (timer->cur_time > timer->next_time) return (0);

    return (timer->next_time - timer->cur_time);
}
#endif

//=============================================================================
#ifndef __FREESCALE_MQX__
bool_t MWA_ElapseTimer(MWA_TimerHandle_t *timer, uint32_t milli_sec)
{
    if (timer == NULL) return (FALSE);

    if (timer->cur_time == 0) return (FALSE);

    timer->cur_time += milli_sec;
    if (timer->cur_time < timer->next_time) {
        return (FALSE);
    }

    timer->cur_time = 0;
    timer->next_time = 0;

    return (TRUE);
}
#endif

//=============================================================================
void MWA_InvokeTimerCallback(MWA_TimerHandle_t *timer)
{
    if (timer == NULL) return;

    if (timer->func != NULL) {
        (*timer->func)(timer->data, timer->tag, FALSE);
    }
}

//=============================================================================
uint32_t MWA_GetMilliSec(void)
{
#ifndef __FREESCALE_MQX__
    struct timeval tv;
    uint32_t msec;

    tv.tv_sec = 0;
    tv.tv_usec = 0;
    gettimeofday(&tv, NULL);

    msec = tv.tv_sec * 1000;
    msec += tv.tv_usec / 1000;

    return (msec);
#else // __FREESCALE_MQX__
    TIME_STRUCT cur_time;

    _time_get(&cur_time);

    return (cur_time.MILLISECONDS);
#endif
}

//=============================================================================
void MWA_WaitMilliSec(uint32_t milli_sec)
{
#ifndef __FREESCALE_MQX__
    usleep(milli_sec * 1000L);
#else // __FREESCALE_MQX__
    _time_delay(milli_sec);
#endif
}

/******************************** END-OF-FILE ********************************/
