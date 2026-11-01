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

    Debug Server DAP - Craig Allsop 2026
*/

#include "CommandQueue.h"

namespace VCC
{
    //
    // Command queue on reset.
    //
    CommandQueue gEmuOnReset;

    //
    // Command queue for emulator thread.
    //
    CommandQueue gEmuCommand;

    ////////////////////////////////////////////////////////////////////////////

    void CommandQueue::Queue(CommandFunc func)
    {
        Lock lock(mCommandsLock);
        mCommandsQueued.push_back(func);
    }

    ////////////////////////////////////////////////////////////////////////////

    bool CommandQueue::Execute()
    {
        Lock lock(mCommandsLock);
        bool result = mCommandsQueued.size() > 0;
        for (auto& f : mCommandsQueued)
            f();
        mCommandsQueued.clear();
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////

    bool CommandQueue::ExecuteNext()
    {
        CommandFunc func{};

        // get next command from queue
        {
            Lock lock(mCommandsLock);
            if (mCommandsQueued.empty()) return false;
            func = mCommandsQueued.front();
            mCommandsQueued.erase(mCommandsQueued.begin());
        }

        // execute outside of lock
        func();
        return true;
    }
}
