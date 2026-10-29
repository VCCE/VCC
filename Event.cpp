#include "BuildConfig.h"
#include "Event.h"
#include <assert.h>

namespace VCC
{
    Event gEvents;
    Event::Event() : 
        mClocks(0), 
        mClocksPerCycle(DefaultClocksPerCycle),
        mReqClocksPerCycle(DefaultClocksPerCycle),
        mResolution(256),
        mClocksPerCycleWithOC(DefaultClocksPerCycle * mResolution),
        mReqOverclock(1),
        mOverclock(1)
    {}

    //
    // do complete reset
    //
    void Event::Reset()
    {
        mClocks = 0;
        mOverclock = 1;
        mClocksPerCycle = DefaultClocksPerCycle;
        mClocksPerCycleWithOC = DefaultClocksPerCycle * mResolution;
        mEvents.clear();
    }

    //
    // set current clocks per cycle
    //
    void Event::SetClocksPerCycle(Event::Clocks c)
    {
        assert(c > 0);
        mReqClocksPerCycle = c;
    }

    //
    // set an overclock factor, 1..64
    //
    void Event::SetOverclock(size_t factor)
    {
        assert(factor > 0);
        mReqOverclock = factor;
    }

    //
    // set the multiplier factor for a tick, default is 256.
    //
    void Event::SetResolution(size_t factor)
    {
        mResolution = factor;
        mClocksPerCycleWithOC = DefaultClocksPerCycle * mResolution;
    }

    //
    // rebase with clocks = 0 and adjust all pending events
    //
    void Event::Rebase()
    {
        auto now = mClocks;

        // reset to zero
        mClocks -= now;

        // adjust pending events
        for (auto& e : mEvents)
            e.time -= now;

        mOverclock = mReqOverclock;
        mClocksPerCycle = mReqClocksPerCycle * mResolution;
        mClocksPerCycleWithOC = mClocksPerCycle / mOverclock;
    }

    //
    // queue function in dt from now
    //
    void Event::Queue(Clocks dtClocks, Func func, size_t id)
    {
        // insert with increasing order
        auto time = mClocks + dtClocks * mResolution;
        QueueInternal(time, func, id);
    }

    //
    // Queue up func at dt in hz from now
    //
    void Event::Queue(Hz dtHz, Func func, size_t id)
    {
        // insert with increasing order
        auto dt = 14318180ll * mResolution / dtHz.value;
        assert(dt < SIZE_MAX);
        auto time = mClocks + (InternalClocks)dt;
        QueueInternal(time, func, id);
    }

    //
    // Insert in order after existing elements
    //
    void Event::QueueInternal(size_t time, Func func, size_t id)
    {
        auto where = std::lower_bound(mEvents.begin(), mEvents.end(), time, [](auto& e, auto t) { return e.time <= t; });
        mEvents.insert(where, { id,time,func });
    }

    //
    // Erase event by its id
    //
    void Event::Erase(size_t id)
    {
        if (id == 0) return;
        auto found = std::find_if(mEvents.begin(), mEvents.end(), [id](auto& e) { return e.id == id; });
        if (found != mEvents.end())
            mEvents.erase(found);
    }

    //
    // Reschedule 'id' from now, if id not found just queue.
    //
    void Event::ReQueue(Clocks dtClocks, Func func, size_t id)
    {
        if (id == 0) return;
        Erase(id);
        Queue(dtClocks, func, id);
    }

    //
    // Tick clock forward by cycles processing any events it passed.
    // note: events may insert events.
    // note: < because we tick prior to advancing the clock, i.e. events
    // that occurred between t.before and up to but not including t.now. 
    //
    void Event::Tick(Cycles cycles)
    {
        auto clocks = cycles * mClocksPerCycleWithOC;

        // while events less than time + cycles
        while (!mEvents.empty() && mEvents[0].time < mClocks + clocks)
        {
            auto dt = mEvents[0].time - mClocks;

            // advance clock so reentry calls have correct state
            mClocks += dt;
            
            // reduce remaining by same amount
            clocks -= dt;

            // get function, call after erasing
            auto func = mEvents[0].func;
            mEvents.erase(mEvents.begin());
            func();
        }

        // advance clock remaining clocks
        mClocks += clocks;
    }

    //
    // return the current clocks
    //
    Event::Clocks Event::GetClocks() const
    {
        return mClocks / mResolution;
    }

    //
    // return the number of events queued
    //
    size_t Event::GetSize() const
    {
        return mEvents.size();
    }

#if _DEBUG
    //
    // read only access for debugging
    //
    const Event::EventItem& Event::operator[](size_t i) const
    {
        return mEvents[i];
    }

    const Event::EventsVector& Event::Events() const
    {
        return mEvents;
    }
#endif

#if USE_DEBUG_TESTS
    //
    // testing
    //
    bool TestEvents()
    {
        struct State { int c = 0; Event e; State() { e.SetResolution(1); } };

        // helpers
        auto nop = []() {}; // empty func
        auto snop = [](State&) {}; // do nothing with state
        auto c = [](State& s, auto func) { return [&]() { s.c++; func(); }; }; // count calls
        auto f = [](auto ...func) { return [=](State& s) { (func(s), ...); }; }; // run sequence of funcs
        auto t = [](auto check, auto func) { State s; func(s); return check(s); }; // run func & check
        auto isSorted = [](auto& v, auto q) { for (auto f = v.begin(), i = v.begin(); ++i != v.end(); ++f) if (q(i) < q(f)) return false; return true; };

        // operations
        auto insert1 = [=](State& s) { s.e.Queue(125, c(s, nop)); };
        auto insert2 = [=](State& s) { s.e.Queue(128, c(s, nop)); };
        auto insert3 = [=](State& s) { s.e.Queue(133, c(s, nop)); };
        auto insert4 = [=](State& s) { s.e.Queue(135, c(s, nop)); };
        auto insert5 = [=](State& s) { s.e.Queue(150, c(s, nop)); };
        auto tick = [](int n) { return [n](State& s) { s.e.Tick(n); }; };
        auto reset = [=]() { return [=](State& s) { s.e.Rebase(); }; };
        auto insertBunch = f(insert1, insert2, insert3);
        auto insertOutOfOrder1 = f(insert2, insert1, insert3);
        auto insertOutOfOrder2 = f(insert3, insert2, insert1);
        auto resetInCall = [](State& s) { return [&]() { s.e.Rebase(); }; };
        auto insertReset = [=](State& s) { s.e.Queue(127, resetInCall(s)); };
        auto insertInCall = [=](State& s) { return [&]() { s.e.Queue(1, c(s, nop)); }; }; // insert +1 after 133
        auto insertAdd = [=](State& s) { s.e.Queue(133, insertInCall(s)); };
        auto insertTagged = [=](int n) { return [=](State& s) { s.e.Queue(130, c(s, nop), n); }; };
        auto removeItem = [](int n) { return [n](State& s) { s.e.Erase(n); }; };

        // evaluations
        auto getClocks = [](State& s) { return s.e.GetClocks(); };
        auto getSize = [](State& s) { return s.e.GetSize(); };
        auto getCalls = [](State& s) { return s.c; };
        auto getTime = [](Event::EventsVector::const_iterator& it) { return it->time; };
        auto isSortedTime = [=](auto& v) { return isSorted(v, getTime); };
        auto isOrdered = [=](State& s) { return isSortedTime(s.e.Events()); };
        auto getTimeFirst = [=](State& s) { return getTime(s.e.Events().begin()); };
        auto isPastDue = [=](State& s) { return getTimeFirst(s) < getClocks(s); };
        
        // queries
        auto clocks = [=](auto func) { return t(getClocks, func); };
        auto size = [=](auto func) { return t(getSize, func); };
        auto calls = [=](auto func) { return t(getCalls, func); };
        auto ordered = [=](auto func) { return t(isOrdered, func); };
        auto firstTime = [=](auto func) { return t(getTimeFirst, func); };
        auto past = [=](auto func) { return t(isPastDue, func); };

        // test sequences (^ = clock pos)
        auto test5 = f(insertBunch, tick(5)); // tick(5) ^125,128,133 
        auto test8 = f(insertBunch, tick(8)); // tick(8) 125,^128,133
        auto test9 = f(insertBunch, tick(9)); // tick(9) 125,128,133^
        auto test83 = f(test8, tick(3)); // tick(8,3) 125^,128,133^ 
        auto testReset1 = f(insertReset, insertBunch, tick(8)); // ^125,127(r),128,133 + tick(8) -> ^1,134
        auto testReset2 = f(testReset1, tick(3)); // ^1,134 + tick(3) -> ^
        auto testRemove = f(insertTagged(3), insertOutOfOrder1, removeItem(3));
        auto testRemoved = f(testRemove, tick(10));
        auto testAdded = f(insert1, insertAdd, insert4, tick(9)); //^125,133,135 + tick(9) -> 125,133,a134,135^
        auto testAdd2 = f(insert5, testAdded); //^125,133,135,150 + tick(9) -> 125,133,a134,135^,150 -> 150
        auto testDups = f(insertOutOfOrder1, insertOutOfOrder2);
        auto testAdd3 = f(insert1, tick(1), insert2, tick(2), insert3, tick(9), insert4); //^125,144,181 ..-> ^327

        // tests
        assert(clocks(snop) == 0);  		// clocks begins at zero
        assert(size(snop) == 0);        	// queue initialized at zero
        assert(calls(snop) == 0);       	// calls initialized at zero
        assert(size(insertBunch) == 3); 	// test queuing 3 has size = 3
        assert(ordered(insertBunch));     	// ensure in order insert is ordered
        assert(ordered(insertOutOfOrder1)); // ensure out of order insert is ordered
        assert(ordered(insertOutOfOrder2)); // ensure out of order insert is ordered
        assert(ordered(testRemove));        // ensure in order after removal
        assert(calls(testRemoved) == 3);    // 4-1 then 3 called
        assert(size(testRemoved) == 0);     // advanced 10 cycles all processed
        assert(clocks(test5) == 80);		// advance 5 cycles = 128 clocks
        assert(size(test8) == 2);     		// advance 8 cycles = 128 clocks = process 1 event leaves 1
        assert(calls(test8) == 1);    		// advance 8 cycles = 128 clocks = process 1 event calls 2
        assert(size(test9) == 0);     		// advance 8 cycles = 128 clocks, leaves 0
        assert(calls(test9) == 3);    		// advance 9 cycles = 144 clocks calls 3
        assert(clocks(test9) == 144);       // advance 9 cycles = 144 clocks
        assert(size(test83) == 0);    		// advance 8,3 cycles = 176 clocks, leaves 0
        assert(calls(test83) == 3);   		// advance 8,3 cycles = 176 clocks, calls 3
        assert(clocks(testReset1) == 1);	// advance 128 clocks with reset at 127 = 1
        assert(calls(testReset1) == 1);		// advance 128 clocks with reset at 127, 2 calls 
        assert(size(testReset1) == 2);		// advance 128 clocks with reset at 127, 1 remaining
        assert(firstTime(testReset1) == 1); // after reset at 127 first element's time should be 1
        assert(clocks(testReset2) == 49);	// advance 176 clocks with reset at 127 = 1+48
        assert(calls(testReset2) == 3);		// advance 176 clocks with reset at 127, 3 calls 
        assert(size(testReset2) == 0);		// advance 176 clocks with reset at 127, 0 remaining
        assert(ordered(testDups));          // ensure duplicates are ordered
        assert(size(testDups) == 6);        // ensure duplicates are all inserted
        assert(calls(testAdded) == 3);      // ensure added during tick gets called
        assert(size(testAdded) == 0);       // ensure added during tick none left
        assert(calls(testAdd2) == 3);       // ensure added during tick gets called
        assert(size(testAdd2) == 1);        // ensure added during tick one remains
        assert(firstTime(testAdd2) == 150); // ensure last remains is the 4th
        assert(past(testAdd2) == false);    // ensure next element is not due
        assert(size(testAdd3) == 1);        // multiple ticks/queues one left
        assert(calls(testAdd3) == 3);       // multiple ticks/queues called 3
        assert(past(testAdd3) == false);    // last is not already due
        assert(firstTime(testAdd3) == 327); // ensure last is 327 (t192+q135)
        return 0;
    }
    static bool gTestEvents = TestEvents();
#endif // USE_DEBUG_TESTS
}