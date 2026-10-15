#pragma once

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

    struct Event
    {
        // clocks @ 14.31818mhz (NTSC)
        using Clocks = size_t;
        static constexpr Clocks ClockFreq = 14318180;

        // cycles = clocks/16
        using Cycles = size_t;
        static constexpr size_t DefaultClocksPerCycle = 16;

        struct Hz { size_t value; };
        using Func = std::function<void()>;
        template <typename T> using Vector = std::vector<T>;
        
        struct EventItem
        {
            size_t id;
            Clocks time;
            Func func;
        };

        using EventsVector = Vector<EventItem>;

        Event();

        //
        // start a new frame, with clocks = 0
        //
        void BeginFrame();

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
        // return the current clocks
        //
        Clocks GetClocks() const;

        //
        // return the number of events queued
        //
        size_t GetSize() const;

        //
        // set current clocks per cycle
        //
        void SetClocksPerCycle(Clocks c);

        //
        // set an overclock factor, 1..64
        //
        void SetOverclock(size_t factor);

        //
        // do complete reset
        //
        void Reset();

#ifdef _DEBUG
        //
        // read only access for debugging
        //
        const EventItem& operator[](size_t i) const;
        const EventsVector& Events() const;
#endif

    private:

        // internally use multiplier of 256 to support overclocking
        using InternalClocks = size_t;
        static const size_t Factor = 256;

        size_t mOverclock;
        InternalClocks mClocks;
        InternalClocks mClocksPerCycleWithOC;
        InternalClocks mClocksPerCycle;
        EventsVector mEvents;

        size_t mReqOverclock;
        size_t mReqClocksPerCycle;
    };

    extern Event gEvents;
}

