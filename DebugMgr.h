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
#include "Dap.h"
#include "CommandQueue.h"
#include "SourceMgr.h"

#include <map>

//
// Hook into debug manager. A session is active when debugger is attached
// this global is set via the emulator thread so as not to conflict.
//
#define DEBUGMGR(...) do { if (VCC::DebugMgr::gDebugMgrSession) VCC::DebugMgr::gDebugMgrSession->__VA_ARGS__; } while (false)

namespace VCC
{
    //
    // VCC debug adaptor manager. Handles the requests from the attached
    // debugger. On attach VCC is reset, binary is loaded, listing is parsed,
    // breakpoints are set. On disconnection VCC is reset, ready for next.
    //
    struct DebugMgr : Dap
    {
        using Base = Dap;
        using Address = SourceMgr::Address;
        using LineId = SourceMgr::LineId;
        using FileId = SourceMgr::FileId;
        template <typename A, typename B> using Map = std::map<A, B>;
        using SV = StringView;

        static constexpr Address naddress = (Address)-1;
        static constexpr LineId nline = (LineId)-1;
        static constexpr FileId nfile = (FileId)-1;

        struct Breakpoint
        {
            int Id;                     // breakpoint identifier from debugger
            FileId File;                // file of breakpoint
            LineId Line;                // desired line of breakpoint
        };

        struct Stack
        {
            Address Dest;               // destination of call
            Address Source;             // origin of call
            Address Next;               // next instruction at origin
        };

        using Breakpoints = Map<Address, Breakpoint>;

        //
        // the session
        //
        static DebugMgr* gDebugMgrSession;

        //
        // create debug manager and pass socket connection to use.
        //
        DebugMgr(Socket sock);

        //
        // the hooks 
        //
        void CheckBreakpoints(uint16_t pc);
        void Call(uint16_t to, uint16_t from, uint16_t next);
        void Return(uint16_t to, uint16_t from);
        void SetStack(uint16_t ns, uint16_t os);

    protected:

        void WaitForCommand();
        void HoldForCommand();
        void EventHitBreakpoint();
        void InitFunctions() override;
        void OnIdle() override;
        void OnInitialize() override;
        void OnBeginBreakpoints(const String& sourceFile) override;
        bool OnSetBreakpoint(const String& sourceFile, int lineNo, int id) override;
        bool OnAttach(const String& binaryFile, const Vector<Json>& listings) override;
        void OnConfigurationDone() override;
        void OnDisconnect() override;
        Json OnRegisters() override;
        Json OnVariables(int ref) override;
        size_t OnStackTraceCount() override;
        void OnStackTrace(int i, String& outName, String& outPath, String& outFunc, int& outLine, int& outColumn) override;
        void Capture(Json& args, const String& command, int requestSeq);
        void OnStep() override;
        void OnStepIn() override;
        void OnStepOut() override;
        void OnContinue() override;
        StringPair OnEvaluate(const String& context, int frameId, const String& expression) override;
        Json OnRepl(const String& command, Json& args) override;

    private:
        void QueueEvent(const String& event, const Json& body = Json::Empty);
        Address CpuToReal(uint16_t address);

        CommandQueue gDapCommand{};     // adapter command queue
        SourceMgr mSourceMgr;           // source listing manager class
        Breakpoints mBreakpoints;       // table of current breakpoints
        Vector<Stack> mStack;           // current mirror of stack
        Address mTOS;                   // top of stack address
        bool mStop{};                   // stop/go state
        bool mStopAtStart{};            // hold until debugger ready
        bool mStopCall{};               // stop at next call
        bool mStopRts{};                // stop at next return if step out
        bool mStackedCurrent{};         // true when breakpoint location added to stack
        LineId mCurrentLine{};          // current line of breakpoint
        FileId mCurrentFile{};          // current file of breakpoint
        Address mStopAt{};              // temporary stop location 1
        Address mStopAt2{};             // temporary stop location 2
        Address mBreakAt{};             // actual address stopped at
    };

    //
    // global entry point for enable/disable debug server.
    //
    void EnableDebugServer(bool on, int port = 0);
}
