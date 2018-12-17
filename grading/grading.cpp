/**
 * @file   grading.cpp
 * @author Sébastien Rouault <sebastien.rouault@epfl.ch>
 *
 * @section LICENSE
 *
 * Copyright © 2018 Sébastien Rouault.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version. Please see https://gnu.org/licenses/gpl.html
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * @section DESCRIPTION
 *
 * Grading of the implementations.
**/

// External headers
#include <algorithm>
#include <atomic>
#include <cstring>
#include <iostream>
#include <random>

// Internal headers
#include "common.hpp"
#include "transactional.hpp"
#include "workload.hpp"

// -------------------------------------------------------------------------- //

/** Tailored thread synchronization class.
**/
class Sync final {
private:
    /** Synchronization status.
    **/
    enum class Status {
        Wait,  // Workers waiting each others, run as soon as all ready
        Run,   // Workers running (still full success)
        Abort, // Workers running (>0 failure)
        Done,  // Workers done (all success)
        Fail,  // Workers done (>0 failure)
        Quit   // Workers must terminate
    };
private:
    unsigned int const        nbworkers; // Number of workers to support
    ::std::atomic<unsigned int> nbready; // Number of thread having reached that state
    ::std::atomic<Status>       status;  // Current synchronization status
    ::std::atomic<char const*>  errmsg;  // Any one of the error message(s)
public:
    /** Deleted copy constructor/assignment.
    **/
    Sync(Sync const&) = delete;
    Sync& operator=(Sync const&) = delete;
    /** Worker count constructor.
     * @param nbworkers Number of workers to support
    **/
    Sync(unsigned int nbworkers): nbworkers{nbworkers}, nbready{0}, status{Status::Done}, errmsg{nullptr} {}
public:
    /** Master trigger "synchronized" execution in all threads (instead of joining).
    **/
    void master_notify() noexcept {
        status.store(Status::Wait, ::std::memory_order_relaxed);
    }
    /** Master trigger termination in all threads (instead of notifying).
    **/
    void master_join() noexcept {
        status.store(Status::Quit, ::std::memory_order_relaxed);
    }
    /** Master wait for all workers to finish.
     * @param maxtick Maximum number of ticks to wait before exiting the process on an error (optional, 'invalid_tick' for none)
     * @return Error constant null-terminated string ('nullptr' for none)
    **/
    char const* master_wait(Chrono::Tick maxtick = Chrono::invalid_tick) {
        Chrono chrono;
        chrono.start();
        while (true) {
            switch (status.load(::std::memory_order_acquire)) { // Synchronize-with 'worker_notify' (in case of shared memory)
            case Status::Done:
                return nullptr;
            case Status::Fail:
                return errmsg;
            default:
                long_pause();
                if (maxtick != Chrono::invalid_tick && chrono.delta() > maxtick)
                    throw Exception::BoundedOverrun{"transactional library takes too long to process the transactions"};
            }
        }
    }
    /** Worker wait until next run.
     * @return Whether the worker can proceed, or quit otherwise
    **/
    bool worker_wait() noexcept {
        while (true) {
            auto res = status.load(::std::memory_order_relaxed);
            if (res == Status::Wait)
                break;
            if (res == Status::Quit)
                return false;
            short_pause();
        }
        auto res = nbready.fetch_add(1, ::std::memory_order_relaxed);
        if (res + 1 == nbworkers) { // Latest worker, switch to run status
            nbready.store(0, ::std::memory_order_relaxed);
            status.store(Status::Run, ::std::memory_order_release); // Synchronize-with previous worker waiting for run/abort state
        } else do { // Not latest worker, wait for run status
            short_pause();
            auto res = status.load(::std::memory_order_acquire); // Synchronize-with latest worker switching to run/abort state
            if (res == Status::Run || res == Status::Abort)
                break;
        } while (true);
        return true;
    }
    /** Worker notify termination of its run.
     * @param error Error constant null-terminated string ('nullptr' for none)
    **/
    void worker_notify(char const* error) noexcept {
        if (error) {
            errmsg.store(error, ::std::memory_order_relaxed);
            status.store(Status::Abort, ::std::memory_order_relaxed);
        }
        auto&& res = nbready.fetch_add(1, ::std::memory_order_acq_rel); // Synchronize-with previous worker(s) potentially setting aborted status
        if (res + 1 == nbworkers) { // Latest worker, switch to done/fail status
            nbready.store(0, ::std::memory_order_relaxed);
            status.store(status.load(::std::memory_order_relaxed) == Status::Abort ? Status::Fail : Status::Done, ::std::memory_order_release); // Synchronize-with 'master_wait' (in case of shared memory)
        }
    }
};

/** Measure the arithmetic mean of the execution time of the given workload with the given transaction library.
 * @param workload     Workload instance to use
 * @param nbthreads    Number of concurrent threads to use
 * @param nbrepeats    Number of repetitions (keep the median)
 * @param seed         Seed to use for performance measurements
 * @param maxtick_init Timeout for (re)initialization ('Chrono::invalid_tick' for none)
 * @param maxtick_perf Timeout for performance measurements ('Chrono::invalid_tick' for none)
 * @param maxtick_chck Timeout for correctness check ('Chrono::invalid_tick' for none)
 * @return Error constant null-terminated string ('nullptr' for none), execution times (in ns) (undefined if inconsistency detected)
**/
static auto measure(Workload& workload, unsigned int const nbthreads, unsigned int const nbrepeats, Seed seed, Chrono::Tick maxtick_init, Chrono::Tick maxtick_perf, Chrono::Tick maxtick_chck) {
    // TODO: Should fork
    ::std::thread threads[nbthreads];
    ::std::mutex  cerrlock;        // To avoid interleaving writes to 'cerr' in case more than one thread throw
    Sync          sync{nbthreads}; // "As-synchronized-as-possible" starts so that threads interfere "as-much-as-possible"
    for (unsigned int i = 0; i < nbthreads; ++i) { // Start threads
        try {
            threads[i] = ::std::thread{[&](unsigned int i) {
                try {
                    // Initialization
                    if (!sync.worker_wait())
                        return;
                    sync.worker_notify(workload.init());
                    // Performance measurements
                    for (unsigned int count = 0; count < nbrepeats; ++count) {
                        if (!sync.worker_wait())
                            return;
                        sync.worker_notify(workload.run(i, seed + nbthreads * count + i));
                    }
                    // Correctness check
                    if (!sync.worker_wait())
                        return;
                    sync.worker_notify(workload.check(i, std::random_device{}())); // Random seed is wanted here
                    // Synchronized quit
                    if (!sync.worker_wait())
                        return;
                    throw Exception::Unreachable{"unexpected worker iteration after checks"};
                } catch (::std::exception const& err) {
                    sync.worker_notify("Internal worker exception(s)"); // Exception post-'Sync::worker_wait' (i.e. in 'Workload::run' or 'Workload::check'), since 'Sync::worker_*' do not throw
                    { // Print the error
                        ::std::unique_lock<decltype(cerrlock)> guard{cerrlock};
                        ::std::cerr << "⎪⎧ *** EXCEPTION - worker thread ***" << ::std::endl << "⎪⎩ " << err.what() << ::std::endl;
                    }
                    return;
                }
            }, i};
        } catch (...) {
            for (unsigned int j = 0; j < i; ++j) // Detach threads to avoid termination due to attached thread going out of scope
                threads[j].detach();
            throw;
        }
    }
    try {
        char const* error = nullptr;
        Chrono::Tick time_init = Chrono::invalid_tick;
        Chrono::Tick times[nbrepeats];
        Chrono::Tick time_chck = Chrono::invalid_tick;
        auto const posmedian = nbrepeats / 2;
        { // Initialization (with cheap correctness test)
            Chrono chrono;
            chrono.start();
            sync.master_notify();
            error = sync.master_wait(maxtick_init);
            if (unlikely(error))
                goto join;
            chrono.stop();
            time_init = chrono.get_tick();
        }
        { // Performance measurements (with cheap correctness tests)
            for (unsigned int i = 0; i < nbrepeats; ++i) {
                Chrono chrono;
                chrono.start();
                sync.master_notify();
                error = sync.master_wait(maxtick_perf);
                if (unlikely(error))
                    goto join;
                chrono.stop();
                times[i] = chrono.get_tick();
            }
            ::std::nth_element(times, times + posmedian, times + nbrepeats); // Partition times around the median
        }
        { // Correctness check
            Chrono chrono;
            chrono.start();
            sync.master_notify();
            error = sync.master_wait(maxtick_chck);
            if (unlikely(error))
                goto join;
            chrono.stop();
            time_chck = chrono.get_tick();
        }
        join: { // Joining
            sync.master_join(); // Join with threads
            for (unsigned int i = 0; i < nbthreads; ++i)
                threads[i].join();
        }
        return ::std::make_tuple(error, time_init, times[posmedian], time_chck);
    } catch (...) {
        for (unsigned int i = 0; i < nbthreads; ++i) // Detach threads to avoid termination due to attached thread going out of scope
            threads[i].detach();
        throw;
    }
}

// -------------------------------------------------------------------------- //

/** Program entry point.
 * @param argc Arguments count
 * @param argv Arguments values
 * @return Program return code
**/
int main(int argc, char** argv) {
    try {
        // Parse command line option(s)
        if (argc < 3) {
            ::std::cout << "Usage: " << (argc > 0 ? argv[0] : "grading") << " [--dynamic] <seed> <reference library path> <tested library path>..." << ::std::endl;
            return 1;
        }
        bool dynamic; // Whether to use dynamic memory allocation
        if (::std::strcmp(argv[1], "--dynamic") == 0) {
            dynamic = true;
            { // Pop the argument
                argv[1] = argv[0];
                --argc;
                ++argv;
            }
        } else {
            dynamic = false;
        }
        // Get/set/compute run parameters
        auto const nbworkers = []() {
            auto res = ::std::thread::hardware_concurrency();
            if (unlikely(res == 0))
                res = 16;
            return static_cast<size_t>(res);
        }();
        auto const nbtxperwrk    = 4000ul / nbworkers;
        auto const nbaccounts    = 32 * nbworkers;
        auto const expnbaccounts = 1024 * nbworkers;
        auto const init_balance  = 100ul;
        auto const prob_long     = dynamic ? 0.05f : 0.5f;
        auto const prob_alloc    = dynamic ? 0.2f : 0.f;
        auto const nbrepeats     = 7;
        auto const seed          = static_cast<Seed>(::std::stoul(argv[1]));
        auto const clk_res       = Chrono::get_resolution();
        auto const slow_factor   = 16ul;
        // Print run parameters
        ::std::cout << "⎧ #worker threads:     " << nbworkers << ::std::endl;
        ::std::cout << "⎪ #TX per worker:      " << nbtxperwrk << ::std::endl;
        ::std::cout << "⎪ #repetitions:        " << nbrepeats << ::std::endl;
        ::std::cout << "⎪ Initial #accounts:   " << nbaccounts << ::std::endl;
        ::std::cout << "⎪ Expected #accounts:  " << expnbaccounts << ::std::endl;
        ::std::cout << "⎪ Initial balance:     " << init_balance << ::std::endl;
        ::std::cout << "⎪ Long TX probability: " << prob_long << ::std::endl;
        ::std::cout << "⎪ Allocation TX prob.: " << prob_alloc << ::std::endl;
        ::std::cout << "⎪ Slow trigger factor: " << slow_factor << ::std::endl;
        ::std::cout << "⎪ Clock resolution:    ";
        if (unlikely(clk_res == Chrono::invalid_tick)) {
            ::std::cout << "<unknown>" << ::std::endl;
        } else {
            ::std::cout << clk_res << " ns" << ::std::endl;
        }
        ::std::cout << "⎩ Seed value:          " << seed << ::std::endl;
        // Library evaluations
        double reference = 0.; // Set to avoid irrelevant '-Wmaybe-uninitialized'
        auto const pertxdiv = static_cast<double>(nbworkers) * static_cast<double>(nbtxperwrk);
        auto maxtick_init = Chrono::invalid_tick;
        auto maxtick_perf = Chrono::invalid_tick;
        auto maxtick_chck = Chrono::invalid_tick;
        for (auto i = 2; i < argc; ++i) {
            try {
                ::std::cout << "⎧ Evaluating '" << argv[i] << "'" << (maxtick_init == Chrono::invalid_tick ? " (reference)" : "") << "..." << ::std::endl;
                // Prepare measurement (load TM library + initialize workload)
                TransactionalLibrary tl{argv[i]};
                WorkloadBank         bank{tl, nbworkers, nbtxperwrk, nbaccounts, expnbaccounts, init_balance, prob_long, prob_alloc};
                // Actual performance measurements and correctness check
                try {
                    auto res = measure(bank, nbworkers, nbrepeats, seed, maxtick_init, maxtick_perf, maxtick_chck);
                    // Check false negative-free correctness
                    auto error = ::std::get<0>(res);
                    if (unlikely(error)) {
                        ::std::cout << "⎩ " << error << ::std::endl;
                        return 1;
                    }
                    // Print results
                    auto tick_init = ::std::get<1>(res);
                    auto tick_perf = ::std::get<2>(res);
                    auto tick_chck = ::std::get<3>(res);
                    auto perfdbl = static_cast<double>(tick_perf);
                    ::std::cout << "⎪ Total user execution time: " << (perfdbl / 1000000.) << " ms";
                    if (maxtick_init == Chrono::invalid_tick) { // Set reference performance
                        maxtick_init = slow_factor * tick_init;
                        if (unlikely(maxtick_init == Chrono::invalid_tick)) // Bad luck...
                            ++maxtick_init;
                        maxtick_perf = slow_factor * tick_perf;
                        if (unlikely(maxtick_perf == Chrono::invalid_tick)) // Bad luck...
                            ++maxtick_perf;
                        maxtick_chck = slow_factor * tick_chck;
                        if (unlikely(maxtick_chck == Chrono::invalid_tick)) // Bad luck...
                            ++maxtick_chck;
                        reference = perfdbl;
                    } else { // Compare with reference performance
                        ::std::cout << " -> " << (reference / perfdbl) << " speedup";
                    }
                    ::std::cout << ::std::endl;
                    ::std::cout << "⎩ Average TX execution time: " << (perfdbl / pertxdiv) << " ns" << ::std::endl;
                } catch (Exception::BoundedOverrun const& err) { // Special case: cannot unload library with running threads, so print error and quick-exit
                    ::std::cerr << "⎪ *** EXCEPTION - main thread ***" << ::std::endl;
                    ::std::cerr << "⎩ " << err.what() << ::std::endl;
                    ::std::abort();
                }
            } catch (::std::exception const& err) {
                ::std::cerr << "⎪ *** EXCEPTION - main thread ***" << ::std::endl;
                ::std::cerr << "⎩ " << err.what() << ::std::endl;
                return 1;
            }
        }
        return 0;
    } catch (::std::exception const& err) {
        ::std::cerr << "⎧ *** EXCEPTION - main thread ***" << ::std::endl << "⎩ " << err.what() << ::std::endl;
        return 1;
    }
}
