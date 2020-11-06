#ifndef __A_BASE_H__
#define __A_BASE_H__

#include <cstdint>
#include <cstdio>
#include <thread>
#include <unordered_map>
#include <utility>

// prefer to use linux syscall (system thread id)
#ifdef linux  // linux platform
#include <sys/syscall.h>
#include <unistd.h>
#define gettid() syscall(SYS_gettid)
#else  // use std::thread::id
std::hash<std::thread::id> ThreadIDHasher;
#define gettid() ThreadIDHasher(std::this_thread::get_id())
#endif  // linux platform

#define LOG(fmt, x...)                                                                           \
    {                                                                                            \
        auto tp = std::chrono::system_clock::now();                                              \
        auto sec = std::chrono::time_point_cast<std::chrono::seconds>(tp);                       \
        auto us = std::chrono::duration_cast<std::chrono::milliseconds>(tp - sec);               \
        auto tt = std::chrono::system_clock::to_time_t(tp);                                      \
        struct tm *ptm = localtime(&tt);                                                         \
        printf("%d-%02d-%02d %02d:%02d:%02d:%03ld ( %zu ) %s: " fmt "\n",                        \
               (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,                \
               (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec, us.count(), gettid(), TAG, \
               ##x);                                                                             \
    }

#define DISALLOW_EVIL_CONSTRUCTORS(className) \
    name(const className &) = delete;         \
    name &operator=(const className &) = delete;

#define DECLARE_NON_COPYASSIGNABLE(className)         \
    className(const className &) = delete;            \
    className(const className &&) = delete;           \
    className &operator=(const className &) = delete; \
    className &operator=(const className &&) = delete;

#define UNIMPLEMENTED_MESSAGE "FUNCTION NOT IMPLEMENTED"

namespace diordna {

template <typename K, typename V>
using KeyedVector = std::unordered_map<K, V>;

using status_t = int32_t;

enum {
    OK,
    NO_ERROR = OK,
    UNKNOWN_ERROR = (-2147483647 - 1),
    NO_MEMORY,
    INVALID_OPERATION,
    BAD_VALUE,
    BAD_TYPE,
    NAME_NOT_FOUND,
    PERMISSION_DENIED,
    NO_INIT,
    ALREADY_EXISTS,
    DEAD_OBJECT,
    FAILED_TRANSACTION,
    BAD_INDEX,
    NOT_ENOUGH_DATA,
    WOULD_BLOCK,
    TIMED_OUT,
    UNKNOWN_TRANSACTION,
    FDS_NOT_ALLOWNED,
    UNEXPECTED_NULL,
};

}  // namespace diordna

#endif  // __A_BASE_H__