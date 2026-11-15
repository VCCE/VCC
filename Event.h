#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include <tuple>

namespace VCC
{
    enum EventTag
    {
        ET_TIMER = 1,
        ET_AUDIO = 2,
    };

    //
    // event queue
    //
    // requires rebase() call to reset clocks to zero at a convenient point in time
    // such as new frame, to avoid integer wrap.
    //
    // defaults to multiple of 256 clock ticks, for accurate hz/overclocking, but not essential.
    //
    // id is optional, only required for deletion/reschedule of events.
    //
    struct Event
    {
        using Clocks = size_t;
        using Cycles = size_t;
        struct Hz { size_t value; };
        using Func = std::function<void()>;

        Event();

        //
        // rebase with clocks = 0
        //
        void Rebase();

        //
        // Queue up func at dt from now
        //
        void Queue(Clocks dtClocks, Func func, size_t id = 0);

        //
        // Queue up func at dt in hz from now
        //
        void Queue(Hz dtHz, Func func, size_t id = 0);

        //
        // Reschedule 'id' from now, if id not found just queue.
        //
        void ReQueue(Clocks dtClocks, Func func, size_t id);

        //
        // Erase event by its id
        //
        void Erase(size_t id);

        //
        // tick clock forward by cycles
        //
        void Tick(Cycles cycles);

        //
        // return the current clock time
        //
        Clocks GetClocks() const;

        //
        // return the number of events queued
        //
        size_t GetSize() const;

        //
        // set current clocks per cycle. default=16
        //
        void SetClocksPerCycle(Clocks c);

        //
        // set an integer CPU clock multiplier (used as a master-clock divider).
        //
        void SetOverclock(size_t factor);

        // Set a rational CPU clock multiplier: numerator / denominator.
        // This keeps fractional overclock rates exact without floating-point drift.
        //
        void SetClockRatio(size_t numerator, size_t denominator);

        //
        // set the resolution (clock multiplier) for a tick, default is 256.
        // only required to be set once before using events.
        //
        void SetResolution(size_t factor);

        //
        // do complete reset
        //
        void Reset();

    private:

        template <typename T> using Vector = std::vector<T>;

        // Internal time uses 64-bit units so the x86 build can safely retain
        // sub-clock precision while converting CPU cycles into master clocks.
        using InternalClocks = std::uint64_t;

        struct EventItem
        {
            size_t id;
            InternalClocks time;
            Func func;
        };

        using EventsVector = Vector<EventItem>;

        // cycles = clocks/16
        static constexpr size_t DefaultClocksPerCycle = 16;

        EventsVector mEvents;                   // events list
        InternalClocks mClocks;                 // current clock position
        InternalClocks mResolution;             // internal units per master clock
        InternalClocks mClocksPerCycle;         // active clocks per cycle, scaled by resolution
        InternalClocks mReqClocksPerCycle;      // next clocks per cycle
        size_t mReqOverclockNumerator;          // next overclock ratio numerator
        size_t mReqOverclockDenominator;        // next overclock ratio denominator
        size_t mOverclockNumerator;             // active overclock ratio numerator
        size_t mOverclockDenominator;           // active overclock ratio denominator
        InternalClocks mClockRemainder;         // fractional clock numerator carried between ticks

        void QueueInternal(InternalClocks dt, Func func, size_t id);

#ifdef _DEBUG
        friend bool TestEvents();

        //
        // read only access for debugging
        //
        const EventItem& operator[](size_t i) const;
        const EventsVector& Events() const;
#endif

    };

    extern Event gEvents;
}

