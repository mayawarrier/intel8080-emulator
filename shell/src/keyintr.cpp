
#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
// any value above 200112L is okay
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>

#if defined(_POSIX_SEMAPHORES)
#include <signal.h>
#include <semaphore.h>
#define EMU_USING_POSIX_KEYINTR 1
#endif

#elif defined(_WIN32)
#include <Windows.h>
#define EMU_USING_WIN32_KEYINTR 1

#else
#include <thread>
#include <chrono>
#endif

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cstdarg>
#include <csignal>
#include <cerrno>

#include "keyintr.hpp"


#ifdef EMU_USING_POSIX_KEYINTR
template <std::size_t N>
static inline ::ssize_t do_write(int fd, const char(&msg)[N])
{
    return ::write(fd, msg, N - 1);
}
// signal-safe
template <std::size_t N>
static void safe_fatal(const char(&msg)[N]) noexcept
{
    do_write(STDERR_FILENO, "\n\033[1;31mi8080emu: fatal: ");
    do_write(STDERR_FILENO, msg);
    do_write(STDERR_FILENO, "\033[0m\n");
    std::abort();
}
#else
static void fatal(const char* format, ...) noexcept
{
    std::va_list args;
    va_start(args, format);
    std::fputs("\n\033[1;33mi8080emu: fatal: ", stderr);
    std::vfprintf(stderr, format, args);
    std::fputs("\033[0m\n", stderr);
    va_end(args);
    std::abort();
}
#endif

#if defined(EMU_USING_POSIX_KEYINTR)
static const int keyintr_sigs[] = { SIGINT, SIGTSTP };
#define NUM_KEYINTR_SIGS 2

static sem_t keyintr_sem;
static struct sigaction keyintr_sa;

static inline bool signals(const int* sigs, int nsigs, void(*handler)(int))
{
    for (int i = 0; i < nsigs; ++i)
        if (std::signal(sigs[i], handler) == SIG_ERR)
            return false;
    return true;
}

// On failure, all handlers are reverted.
static bool sigactions(const int* sigs, int nsigs, 
    struct sigaction* sa, struct sigaction* old_sas)
{
    for (int i = 0; i < nsigs; ++i)
    {
        if (::sigaction(sigs[i], sa, &old_sas[i]) == -1)
        {
            for (int j = 0; j < i; ++j)
                if (::sigaction(sigs[j], &old_sas[j], NULL) == -1)
                    safe_fatal("sigaction revert");
            return false;
        }
    }
    return true;
}

// Silently fails if a thread is already waiting.
static void sem_reset(sem_t* sem)
{
    errno = 0;
    while (::sem_trywait(sem) == -1 && errno == EINTR)
        errno = 0;
    if (!(errno == 0 || errno == EAGAIN))
        safe_fatal("sem_reset");
}

extern "C" void keyintr_handler(int sig)
{
    (void)sig;
    if (!signals(keyintr_sigs, NUM_KEYINTR_SIGS, SIG_IGN))
        safe_fatal("signals SIG_IGN");

    if (::sem_post(&keyintr_sem) == -1)
        safe_fatal("sem_post");
}

bool keyintr_wait(void)
{
    struct sigaction old_sas[NUM_KEYINTR_SIGS];
    if (!sigactions(keyintr_sigs, NUM_KEYINTR_SIGS, &keyintr_sa, old_sas)) {
        sem_reset(&keyintr_sem);
        return false;
    }
    errno = 0;
    while (::sem_wait(&keyintr_sem) == -1 && errno == EINTR)
        errno = 0;
    if (errno)
        safe_fatal("sem_wait");

    // just in case, since there can be a race
    // between multiple invokes of signal/sigaction
    sem_reset(&keyintr_sem);
    return true;
}

#elif defined(EMU_USING_WIN32_KEYINTR)
static HANDLE keyintr_sem;

static BOOL WINAPI keyintr_handler(DWORD event)
{
    if (event == CTRL_C_EVENT)
    {
        if (!SetConsoleCtrlHandler(NULL, TRUE))
            fatal("disable Ctrl+C error %d", GetLastError());

        if (!ReleaseSemaphore(keyintr_sem, 1, NULL))
            fatal("ReleaseSemaphore error %d", GetLastError());
        return TRUE;
    }
    else return FALSE;
}

bool keyintr_wait(void)
{
    if (!SetConsoleCtrlHandler(NULL, FALSE)) // enable Ctrl+C
        return false;

    if (WaitForSingleObject(keyintr_sem, INFINITE) != WAIT_OBJECT_0)
        fatal("WaitForSingleObject error %d", GetLastError());

    return true;
}

#else
static volatile std::sig_atomic_t keyintr;

extern "C" void keyintr_handler(int sig)
{
    std::signal(sig, SIG_IGN);
    keyintr = 1;
}

bool keyintr_wait(void)
{
    if (std::signal(SIGINT, keyintr_handler) == SIG_ERR)
        return false;

    while (keyintr != 1)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    keyintr = 0;
    return true;
}
#endif


bool keyintr_initlzd = false;

bool keyintr_init(void)
{
    assert(!keyintr_initlzd);

#if defined(EMU_USING_POSIX_KEYINTR)
    if (::sem_init(&keyintr_sem, 0, 0) == -1)
        return false;

    keyintr_sa.sa_flags = 0;
    keyintr_sa.sa_handler = keyintr_handler;

    ::sigemptyset(&keyintr_sa.sa_mask);
    for (int i = 0; i < NUM_KEYINTR_SIGS; ++i)
    {
        if (::sigaddset(&keyintr_sa.sa_mask, keyintr_sigs[i]) == -1)
        {
            if (::sem_destroy(&keyintr_sem) == -1)
                safe_fatal("sem_destroy");
            return false;
        }
    }
#elif defined(EMU_USING_WIN32_KEYINTR)
    keyintr_sem = CreateSemaphore(NULL, 0, 1, NULL);
    if (!keyintr_sem)
        return false;
#else
    keyintr = 0;
#endif
    keyintr_initlzd = true;
    return true;
}

void keyintr_destroy(void)
{
    assert(keyintr_initlzd);
    if (keyintr_sigsset)
        keyintr_resetsigs();

#if defined(EMU_USING_POSIX_KEYINTR)
    if (::sem_destroy(&keyintr_sem) == -1)
        safe_fatal("sem_destroy");

#elif defined(EMU_USING_WIN32_KEYINTR)
    if (!CloseHandle(keyintr_sem))
        fatal("CloseHandle error %d", GetLastError());
#else
    keyintr = 0;
#endif
}

bool keyintr_sigsset = false;

bool keyintr_setsigs(void)
{
    assert(keyintr_initlzd && !keyintr_sigsset);

#if defined(EMU_USING_POSIX_KEYINTR)
    if (!signals(keyintr_sigs, NUM_KEYINTR_SIGS, SIG_IGN)) {
        keyintr_resetsigs();
        return false;
    }
#elif defined(EMU_USING_WIN32_KEYINTR)
    if (!SetConsoleCtrlHandler(NULL, TRUE))
        return false;

    // add the handler once. resetting the handler after each signal
    // is not possible as the handler runs on a new thread
    if (!SetConsoleCtrlHandler(keyintr_handler, TRUE)) {
        keyintr_resetsigs();
        return false;
    }
#else
    if (std::signal(SIGINT, SIG_IGN) == SIG_ERR)
        return false;
#endif
    keyintr_sigsset = true;
    return true;
}

void keyintr_resetsigs(void)
{
    assert(keyintr_initlzd && keyintr_sigsset);

#if defined(EMU_USING_POSIX_KEYINTR)
    if (!signals(keyintr_sigs, NUM_KEYINTR_SIGS, SIG_DFL))
        safe_fatal("signals SIG_DFL");

#elif defined(EMU_USING_WIN32_KEYINTR)
    if (!SetConsoleCtrlHandler(keyintr_handler, FALSE))
        fatal("remove handler error %d", GetLastError());

    if (!SetConsoleCtrlHandler(NULL, FALSE))
        fatal("enable Ctrl+C error %d", GetLastError());
#else
    if (std::signal(SIGINT, SIG_DFL) == SIG_ERR)
        fatal("signal SIG_DFL");
#endif
    keyintr_sigsset = false;
}
