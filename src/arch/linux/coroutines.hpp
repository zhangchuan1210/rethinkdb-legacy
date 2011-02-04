#ifndef __ARCH_LINUX_COROUTINES_HPP__
#define __ARCH_LINUX_COROUTINES_HPP__

#include "utils2.hpp"
#include <list>
#include <vector>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include "arch/linux/message_hub.hpp"

const size_t MAX_COROUTINE_STACK_SIZE = 8*1024*1024;

struct coro_context_t;

/* Please only construct one coro_globals_t per thread. Coroutines can only be used when
a coro_globals_t exists. It exists to take advantage of RAII. */

struct coro_globals_t {

    coro_globals_t();
    ~coro_globals_t();
};

/* A coro_t represents a fiber of execution within a thread. Create one with spawn(). Within a
coroutine, call wait() to return control to the scheduler; the coroutine will be resumed when
another fiber calls notify() on it.

coro_t objects can switch threads with move_to_thread(), but it is recommended that you use
on_thread_t for more safety. */

struct coro_t :
    private linux_thread_message_t
{
    friend class coro_context_t;
    friend bool is_coroutine_stack_overflow(void *);

public:
    template<typename callable_t>
    static void spawn(const callable_t& fun) {
        new coro_t(fun);
    }

    // Use coro_t::spawn(boost::bind(...)) for multiparamater spawnings.

public:
    static void wait(); //Pauses the current coroutine until it's notified
    static coro_t *self(); //Returns the current coroutine
    void notify(); //Wakes up the coroutine, allowing the scheduler to trigger it to continue
    static void move_to_thread(int thread); //Wait and notify self on the CPU (avoiding race conditions)

public:
    static void set_coroutine_stack_size(size_t size);

private:
    /* Internally, we recycle ucontexts and stacks. So the real guts of the coroutine are in the
    coro_context_t object. */
    coro_context_t *context;

    coro_t(const boost::function<void()>& deed);
    boost::function<void()> deed_;
    void run();
    ~coro_t();

    virtual void on_thread_switch();

    int current_thread_;

    // Sanity check variable
    bool notified_;

    DISABLE_COPYING(coro_t);
};

/* Returns true if the given address is in the protection page of the current coroutine. */
bool is_coroutine_stack_overflow(void *addr);

#endif // __ARCH_LINUX_COROUTINES_HPP__
