
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
    do_write(STDERR_FILENO, "\n\033[1;33mi8080emu: fatal: ");
    do_write(STDERR_FILENO, msg);
    do_write(STDERR_FILENO, "\033[0m\n");
    std::abort();
}
#endif
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

#if defined(EMU_USING_POSIX_KEYINTR)
static const int keyintr_sigs[] = { SIGINT, SIGTSTP };
#define NUM_KEYINTR_SIGS 2

static sem_t keyintr_sem;
static struct sigaction keyintr_sa;

static bool signals(const int* sigs, int nsigs, void(*handler)(int))
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
                    fatal("sigaction revert errno %d", errno);
            return false;
        }
    }
    return true;
}

// Silently fails if a thread is already waiting.
static void sem_reset(sem_t* sem)
{
    while (true)
    {
        int value;
        if (::sem_getvalue(sem, &value) == -1)
            fatal("sem_getvalue errno %d", errno);

        if (value > 0)
        {
            errno = 0;
            while (::sem_trywait(sem) == -1 && errno == EINTR)
                errno = 0;
            if (!(errno == 0 || errno == EAGAIN))
                fatal("sem_trywait errno %d", errno);
        }
        else break;
    }
}

extern "C" void keyintr_handler(int sig) noexcept
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
        fatal("sem_wait errno %d", errno);

    // just in case, as there can be a race between
    // signal types before all are set to SIG_IGN
    sem_reset(&keyintr_sem);
    return true;
}

#elif defined(EMU_USING_WIN32_KEYINTR)
static HANDLE keyintr_sem;

static BOOL WINAPI keyintr_handler(DWORD event) noexcept
{
    if (event == CTRL_C_EVENT)
    {
        if (!SetConsoleCtrlHandler(NULL, TRUE))
            fatal("win32 disable Ctrl+C error %d", GetLastError());

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

extern "C" void keyintr_handler(int sig) noexcept
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
static bool keyintr_active = false;

static void keyintr_atexit() noexcept
{
    if (keyintr_initlzd)
    {
#if defined(EMU_USING_POSIX_KEYINTR)
        sem_reset(&keyintr_sem);
        if (::sem_destroy(&keyintr_sem) == -1)
            fatal("sem_destroy errno %d", errno);

#elif defined(EMU_USING_WIN32_KEYINTR)
        if (!CloseHandle(keyintr_sem))
            fatal("CloseHandle error %d", GetLastError());
#endif
    }
}

bool keyintr_init(void)
{
    assert(!keyintr_initlzd && !keyintr_active);

    // failing to destroy a semaphore on Linux
    // can lead to a system-wide resource leak!
    if (std::atexit(keyintr_atexit) != 0)
        return false;

#if defined(EMU_USING_POSIX_KEYINTR)
    if (::sem_init(&keyintr_sem, 0, 0) == -1)
        return false;

    keyintr_sa.sa_flags = 0;
    keyintr_sa.sa_handler = keyintr_handler;

    if (::sigemptyset(&keyintr_sa.sa_mask) == -1)
        fatal("sigemptyset errno %d", errno);

    for (int i = 0; i < NUM_KEYINTR_SIGS; ++i)
    {
        if (::sigaddset(&keyintr_sa.sa_mask, keyintr_sigs[i]) == -1)
        {
            if (::sem_destroy(&keyintr_sem) == -1)
                fatal("sem_destroy errno %d", errno);
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

bool keyintr_start(void)
{
    assert(keyintr_initlzd && !keyintr_active);

#if defined(EMU_USING_POSIX_KEYINTR)
    if (!signals(keyintr_sigs, NUM_KEYINTR_SIGS, SIG_IGN)) {
        keyintr_end();
        return false;
    }
#elif defined(EMU_USING_WIN32_KEYINTR)
    if (!SetConsoleCtrlHandler(NULL, TRUE))
        return false;

    // add the handler once. resetting the handler after each signal
    // is not possible as the handler runs on a new thread
    if (!SetConsoleCtrlHandler(keyintr_handler, TRUE)) {
        keyintr_end();
        return false;
    }
#else
    if (std::signal(SIGINT, SIG_IGN) == SIG_ERR)
        return false;
#endif
    keyintr_active = true;
    return true;
}

void keyintr_end(void)
{
    assert(keyintr_initlzd && keyintr_active);

#if defined(EMU_USING_POSIX_KEYINTR)
    if (!signals(keyintr_sigs, NUM_KEYINTR_SIGS, SIG_DFL))
        fatal("signals SIG_DFL errno %d", errno);

#elif defined(EMU_USING_WIN32_KEYINTR)
    if (!SetConsoleCtrlHandler(keyintr_handler, FALSE))
        fatal("win32 remove keyintr_handler error %d", GetLastError());

    if (!SetConsoleCtrlHandler(NULL, FALSE))
        fatal("win32 enable Ctrl+C error %d", GetLastError());
#else
    if (std::signal(SIGINT, SIG_DFL) == SIG_ERR)
        fatal("signal SIG_DFL");
#endif
    keyintr_active = false;
}
