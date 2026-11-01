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
#include "Socket.h"
#include "CommandQueue.h"
#include "Json.h"

#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <map>
#include <tuple>

namespace VCC
{
    struct Dap
    {
        using String = std::string;
        using StringView = std::string_view;
        using AtmoicBool = std::atomic<bool>;
        template <typename ...T> using Map = std::map<T...>;
        template <typename ...T> using Pair = std::pair<T...>;
        template <typename T> using Function = std::function<T>;
        template <typename T> using UniquePtr = std::unique_ptr<T>;
        template <typename T> using Vector = std::vector<T>;
        using DispatchFunc = Function<void(Json&, const String&, int)>;
        using DapPtr = UniquePtr<Dap>;
        using Table = Map<String, DispatchFunc>;
        using StringPair = Pair<String, String>;

        Dap(Socket client);
        virtual ~Dap();

        //
        // main loop for dap
        //
        static void Main(int port, AtmoicBool& stop, Function<DapPtr(Socket)> create);

    protected:

        //
        // add a function to dispatch table
        //
        void AddFunction(const String& command, DispatchFunc func);

        //
        // ininitalize dap dispatch functions
        //
        virtual void InitFunctions();

        //
        // process events/sending
        //
        virtual void OnIdle() {}

        //
        // initialize a debug session
        //
        virtual void OnInitialize() {}

        //
        // begin update of breakpoints for this source file
        //
        virtual void OnBeginBreakpoints(const String& sourceFile) {}

        //
        // client sends set breakpoint message
        //
        virtual bool OnSetBreakpoint(const String& sourceFile, int lineNo, int id) { return false; }

        //
        // client attaches to launch binary file
        //
        virtual bool OnAttach(const String& binaryFile, const String& paste, const String& entryPoint, bool reset, const Vector<Json>& listings) { return false; }

        //
        // when debugger has finished with configuration.
        //
        virtual void OnConfigurationDone() {}

        //
        // read registers
        //
        virtual Json OnRegisters() { return {}; }

        //
        // read registers
        //
        virtual Json OnStack() { return {}; }

        //
        // read variables
        //
        virtual Json OnVariables(int ref) { return {}; }

        //
        // user step over (f10)
        //
        virtual void OnStep() {}

        //
        // user step in (f11)
        //
        virtual void OnStepIn() {}

        //
        // user step out (shift-f11)
        //
        virtual void OnStepOut() {}

        //
        // user continue (f5)
        //
        virtual void OnContinue() {}

        //
        // when client disconnects/ends debugging
        //
        virtual void OnDisconnect() {}

        //
        // stack depth
        //
        virtual size_t OnStackTraceCount() { return 0; }

        //
        // get stack frame i'th index, 0 = top
        //
        virtual void OnStackTrace(int i, String& outName, String& outPath, String& outFunc, int& outLine, int& outColumn) {}

        //
        // perform evaluate on expression
        //
        virtual StringPair OnEvaluate(const String& context, int frameId, const String& expression) { return {}; }

        //
        // perform repl command
        //
        virtual Json OnRepl(const String& command, Json& args) { return {}; }

        //
        // send an 'event' response to debugger
        //
        void SendEvent(const String& event, const Json& body = Json::Empty);

        //
        // send a normal answer to debugger
        //
        void SendResponse(int reqSeq, const String& command, const Json& body = Json::Empty);

        //
        // send error condition to debugger
        //
        void SendError(int reqSeq, const String& command, const String& message);

    private:

        CommandQueue mQueuedCommands;
        Socket mClient;                 // socket use for comms
        Table mDispatch;                // dispatch table
        int mSeq;                       // next sequence id
        int mBpIndex;

        bool DataWaiting();
        Json ReadMessage();
        void WriteMessage(const Json& msg);
        void Run(AtmoicBool& stop);

        //
        // dap command handlers
        //
        void Initialize(Json& args, const String& command, int requestSeq);
        void SetBreakpoints(Json& args, const String& command, int requestSeq);
        void ConfigurationDone(Json& args, const String& command, int requestSeq);
        void Disconnect(Json& args, const String& command, int requestSeq);
        void Attach(Json& args, const String& command, int requestSeq);
        void Threads(Json& args, const String& command, int requestSeq);
        void StackTrace(Json& args, const String& command, int requestSeq);
        void Scopes(Json& args, const String& command, int requestSeq);
        void Variables(Json& args, const String& command, int requestSeq);
        void Next(Json& args, const String& command, int requestSeq);
        void StepIn(Json& args, const String& command, int requestSeq);
        void StepOut(Json& args, const String& command, int requestSeq);
        void Continue(Json& args, const String& command, int requestSeq);
        void Evaluate(Json& args, const String& command, int requestSeq);
    };
}
