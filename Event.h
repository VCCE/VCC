/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

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
        ET_COMMAND = 3,
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
        struct Frames { size_t value; };
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
        // Queue up func in x frames from now
        // (130 max @ 128 resolution)
        //
        void Queue(Frames frames, Func func, size_t id = 0);

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
        // set an overclock factor, 1..64 (clock divider)
        //
        void SetOverclock(size_t factor);

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
        
        struct EventItem
        {
            size_t id;
            Clocks time;
            Func func;
        };

        using EventsVector = Vector<EventItem>;

        // cycles = clocks/16
        static constexpr size_t DefaultClocksPerCycle = 16;

        // internally use multiplier of 256 to support overclocking
        using InternalClocks = size_t;
        
        EventsVector mEvents;                   // events list
        InternalClocks mClocks;                 // current clock position
        InternalClocks mClocksPerCycleWithOC;   // clocks per cycle including overclock
        InternalClocks mClocksPerCycle;         // clocks per cycle
        InternalClocks mReqClocksPerCycle;      // next clocks per cycle
        size_t mReqOverclock;                   // next overclock
        size_t mOverclock;                      // current overclock multiplier
        size_t mResolution;                     // tick resolution

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

