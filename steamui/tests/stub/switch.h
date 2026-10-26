#pragma once
#include <cstdint>
#include <cstddef>
#include <pthread.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef int64_t s64;
typedef u32 Result;
#define R_SUCCEEDED(r) ((r) == 0)
#define R_FAILED(r) ((r) != 0)
#define FS_MAX_PATH 0x301
typedef pthread_mutex_t Mutex; typedef pthread_cond_t CondVar;
inline void mutexInit(Mutex *m){ pthread_mutex_init(m, nullptr); }
inline void mutexLock(Mutex *m){ pthread_mutex_lock(m); }
inline void mutexUnlock(Mutex *m){ pthread_mutex_unlock(m); }
inline void condvarInit(CondVar *c){ pthread_cond_init(c, nullptr); }
inline Result condvarWait(CondVar *c, Mutex *m){ return pthread_cond_wait(c, m); }
inline Result condvarWakeAll(CondVar *c){ return pthread_cond_broadcast(c); }
inline Result condvarWakeOne(CondVar *c){ return pthread_cond_signal(c); }
typedef void (*ThreadFunc)(void*);
struct Thread { pthread_t t; ThreadFunc f; void *a; };
extern int g_ThreadCreateFail;
inline void *_thr_tramp(void *p){ auto t = (Thread*)p; t->f(t->a); return nullptr; }
inline Result threadCreate(Thread *t, ThreadFunc f, void *a, void*, size_t, int, int){ if(g_ThreadCreateFail) return 1; t->f=f; t->a=a; return 0; }
inline Result threadStart(Thread *t){ return pthread_create(&t->t, nullptr, _thr_tramp, t); }
inline Result threadWaitForExit(Thread *t){ return pthread_join(t->t, nullptr); }
inline Result threadClose(Thread*){ return 0; }
