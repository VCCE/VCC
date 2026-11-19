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

#pragma once

#include "BuildConfig.h"
#include <functional>
#include <mutex>

namespace VCC
{
    //
    // execute code on emu thread
    //
    #define QEMU(...) do { gEmuCommand.Queue([=]() { __VA_ARGS__; }); } while (false)
    #define QEMURESET(...) do { gEmuOnReset.Queue([=]() { __VA_ARGS__; }); } while (false)

    //
    // A blocking command queue for running commands between threads.
    //
    struct CommandQueue
    {
        using Mutex = std::mutex;
        using Lock = std::lock_guard<Mutex>;
        template <typename T> using Function = std::function<T>;
        template <typename T> using Vector = std::vector<T>;
        using CommandFunc = Function<void()>;
        using Commands = Vector<CommandFunc>;

        //
        // queue a command
        //
        void Queue(CommandFunc func);

        //
        // execute all the commands queued, true if any executed
        //
        bool Execute();

        //
        // execute next command, true if one executed
        //
        bool ExecuteNext();

        //
        // number of commands queued
        //
        size_t Size() { return mCommandsQueued.size(); }

    private:
        Mutex mCommandsLock;
        Commands mCommandsQueued;
    };

    extern CommandQueue gEmuOnReset;
    extern CommandQueue gEmuCommand;
}