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

#ifndef BITL
#define BITL(n) (1ULL << (n))
#endif
#define HidNpadButton_A (1ULL << 0)
#define HidNpadButton_B (1ULL << 1)
#define HidNpadButton_X (1ULL << 2)
#define HidNpadButton_Y (1ULL << 3)
#define HidNpadButton_StickL (1ULL << 4)
#define HidNpadButton_StickR (1ULL << 5)
#define HidNpadButton_L (1ULL << 6)
#define HidNpadButton_R (1ULL << 7)
#define HidNpadButton_ZL (1ULL << 8)
#define HidNpadButton_ZR (1ULL << 9)
#define HidNpadButton_Plus (1ULL << 10)
#define HidNpadButton_Minus (1ULL << 11)
#define HidNpadButton_Left (1ULL << 12)
#define HidNpadButton_Up (1ULL << 13)
#define HidNpadButton_Right (1ULL << 14)
#define HidNpadButton_Down (1ULL << 15)
#define HidNpadButton_StickLLeft (1ULL << 16)
#define HidNpadButton_StickLRight (1ULL << 18)
#define HidNpadButton_StickRLeft (1ULL << 20)
#define HidNpadButton_StickRRight (1ULL << 22)
