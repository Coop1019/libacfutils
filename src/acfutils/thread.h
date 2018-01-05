/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * CDDL HEADER END
*/
/*
 * Copyright 2017 Saso Kiselkov. All rights reserved.
 */

#ifndef	_ACF_UTILS_THREAD_H_
#define	_ACF_UTILS_THREAD_H_

#include <stdlib.h>

#if APL || LIN
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#else	/* !APL && !LIN */
#include <windows.h>
#endif	/* !APL && !LIN */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Basic portable multi-threading API. We have 4 kinds of objects and
 * associated manipulation functions here:
 * 1) threat_t - A generic thread identification structure.
 * 2) mutex_t - A generic mutual exclusion lock.
 * 3) condvar_t - A generic condition variable.
 */

#if	APL || LIN

#define	thread_t		pthread_t
#define	mutex_t			pthread_mutex_t
#define	condvar_t		pthread_cond_t

#define	mutex_init(mtx)		pthread_mutex_init((mtx), NULL)
#define	mutex_destroy(mtx)	pthread_mutex_destroy((mtx))
#define	mutex_enter(mtx)	pthread_mutex_lock((mtx))
#define	mutex_exit(mtx)		pthread_mutex_unlock((mtx))

#define	thread_create(thrp, proc, arg) \
	(pthread_create(thrp, NULL, (void *(*)(void *))proc, \
	    arg) == 0)
#define	thread_join(thrp)	pthread_join(*(thrp), NULL)

#define	cv_wait(cv, mtx)	pthread_cond_wait((cv), (mtx))
#define	cv_timedwait(cond, mtx, microtime) \
	do { \
		uint64_t t = (microtime); \
		struct timespec ts = { .tv_sec = t / 1000000, \
		    .tv_nsec = (t % 1000000) * 1000 }; \
		(void) pthread_cond_timedwait((cond), (mtx), &ts); \
	} while (0)
#define	cv_init(cv)		pthread_cond_init((cv), NULL)
#define	cv_destroy(cv)		pthread_cond_destroy((cv))
#define	cv_broadcast(cv)	pthread_cond_broadcast((cv))

#else	/* !APL && !LIN */

#define	thread_t	HANDLE
typedef struct {
	bool_t			inited;
	CRITICAL_SECTION	cs;
} mutex_t;
#define	condvar_t	CONDITION_VARIABLE

#define	mutex_init(x) \
	do { \
		(x)->inited = B_TRUE; \
		InitializeCriticalSection(&(x)->cs); \
	} while (0)
#define	mutex_destroy(x) \
	do { \
		ASSERT((x)->inited); \
		DeleteCriticalSection(&(x)->cs); \
		(x)->inited = B_FALSE; \
	} while (0)
#define	mutex_enter(x) \
	do { \
		ASSERT((x)->inited); \
		EnterCriticalSection(&(x)->cs); \
	} while (0)
#define	mutex_exit(x) \
	do { \
		ASSERT((x)->inited); \
		LeaveCriticalSection(&(x)->cs); \
	} while (0)

#define	thread_create(thrp, proc, arg) \
	((*(thrp) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)proc, arg, \
	    0, NULL)) != NULL)
#define	thread_join(thrp) \
	VERIFY3S(WaitForSingleObject(*(thrp), INFINITE), ==, WAIT_OBJECT_0)

#define	cv_wait(cv, mtx) \
	VERIFY(SleepConditionVariableCS((cv), &(mtx)->cs, INFINITE))
#define	cv_timedwait(cv, mtx, limit) \
	do { \
		uint64_t now = microclock(); \
		if (now < (limit)) { \
			(void) SleepConditionVariableCS((cv), &(mtx)->cs, \
			    ((limit) - now) / 1000); \
		} \
	} while (0)
#define	cv_init		InitializeConditionVariable
#define	cv_destroy(cv)	/* no-op */
#define	cv_broadcast	WakeAllConditionVariable

#endif	/* !APL && !LIN */

#ifdef __cplusplus
}
#endif

#endif	/* _ACF_UTILS_THREAD_H_ */
