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

#include "DebugMgr.h"
#include "Event.h"
#include "Base64.h"

#include "defines.h"
#include "quickload.h"
#include "Screenshot.h"
#include "IDisplay.h"
#include "StringUtils.h"

#include <vcc/bus/cartridge_messages.h>
#include <Windows.h>
#include <thread>

#define CFG_DEBUG_DEBUGMGR true

extern SystemState EmuState;
extern VCC::CPUState(*CPUGetState)();

//
// execute code on dap thread
//
#define QDAP(...) do { gDapCommand.Queue([=]() { __VA_ARGS__; }); } while (false)

namespace VCC
{
    extern Event gEvents;
    DebugMgr* DebugMgr::gDebugMgrSession = nullptr;
    DebugMgr::AtmoicBool dapStop{};
    std::thread dapThread{};

    namespace
    {
        auto log = [](const char* fmt, auto ...args)
        {
#if CFG_DEBUG_DEBUGMGR
            static char tmp[65536];
            sprintf(tmp, fmt, args...);
            OutputDebugString(tmp);
#endif
        };
    }


    //
    // Debug Adapter Protocol endpoint
    //
    void EnableDebugServer(bool on, int port)
    {
        if (on)
        {
            // if window recreated
            if (dapThread.joinable()) return;
            dapThread = std::thread([](int port) {
                Dap::Main(port, dapStop, [](Socket sock) { return std::make_unique<DebugMgr>(std::move(sock)); });
            }, port);
        }
        else
        {
            if (dapThread.joinable())
            {
                dapStop = true;
                dapThread.join();
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    namespace
    {
        std::string ToHex(size_t v, int len)
        {
            return "$" + Debugger::ToHexString(v, len);
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    DebugMgr::DebugMgr(Socket sock) : Dap(std::move(sock))
    {
        mStopAt2 = mStopAt = naddress;
        mStop = false;
        mStopAtStart = false;
    }

    ////////////////////////////////////////////////////////////////////////////

    DebugMgr::Address DebugMgr::CpuToReal(uint16_t address)
    {
        uint32_t CpuToRealAddress(uint16_t address);
        return Address{ CpuToRealAddress(address) };
    }

    ////////////////////////////////////////////////////////////////////////////

    // emu thread
    void DebugMgr::Call(uint16_t to, uint16_t from, uint16_t next)
    {
        auto realTo = CpuToReal(to);
        auto realFrom = CpuToReal(from);
        auto realNext = CpuToReal(next);
        mStack.push_back({ realTo, realFrom, realNext });
        if (mStopCall) mStop = true;
    }

    ////////////////////////////////////////////////////////////////////////////

    // emu thread
    void DebugMgr::Return(uint16_t to, uint16_t from)
    {
        auto realTo = CpuToReal(to);
        if (mStack.size() && mStack.back().Next == realTo)
            mStack.pop_back();
    }

    ////////////////////////////////////////////////////////////////////////////

    // emu thread
    void DebugMgr::SetStack(uint16_t ns, uint16_t os)
    {
        mTOS = Address{ ns };
    }

    ////////////////////////////////////////////////////////////////////////////

    // emu thread
    void DebugMgr::CheckBreakpoints(uint16_t pc)
    {
        if (mStopAtStart) HoldForCommand();
        auto address = CpuToReal(pc);
        if (mStop || mStopAt == address || mStopAt2 == address)
        {
            mBreakAt = address;
            mStopAt = naddress;
            mStopAt2 = naddress;
            auto& [fileId, lineId] = mSourceMgr.AddressToSource(address);
            mCurrentFile = fileId;
            mCurrentLine = lineId;
            EventHitBreakpoint();
            WaitForCommand();
        }
        else if (!mStopRts && mBreakpoints.size())
        {
            auto hit = mBreakpoints.find(address);
            if (hit != mBreakpoints.end())
            {
                mBreakAt = address;
                mCurrentFile = hit->second.File;
                mCurrentLine = hit->second.Line;
                EventHitBreakpoint();
                WaitForCommand();
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    // emu thread
    void DebugMgr::WaitForCommand()
    {
        mStopCall = false;
        mStopRts = false;
        mStop = true;
        while (mStop)
            if (!gEmuCommand.ExecuteNext())
                Sleep(10);

        if (mStackedCurrent)
        {
            mStackedCurrent = false;
            mStack.pop_back();
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    // emu thread
    void DebugMgr::HoldForCommand()
    {
        mStopAtStart = true;
        while (mStopAtStart)
            if (!gEmuCommand.ExecuteNext())
                Sleep(10);
    }

    ////////////////////////////////////////////////////////////////////////////

    // emu thread, stop on breakpoint
    void DebugMgr::EventHitBreakpoint()
    {
        mStackedCurrent = true;
        mStack.push_back({ mBreakAt,mBreakAt,mBreakAt });

        auto body = Json::Object{
            { "reason", "breakpoint" },
            { "threadId", 1 },
            { "allThreadsStopped", true },
            { "text", "Breakpoint Hit" }
        };
        QueueEvent("stopped", body);
    }

    ////////////////////////////////////////////////////////////////////////////

    // dap thread
    void DebugMgr::InitFunctions()
    {
        AddFunction("capture", [=](auto ...args) { Capture(args...); });
        Base::InitFunctions();
    }

    ////////////////////////////////////////////////////////////////////////////

    // dap thread
    void DebugMgr::OnIdle()
    {
        gDapCommand.ExecuteNext();
    }

    ////////////////////////////////////////////////////////////////////////////

    void DebugMgr::OnInitialize()
    {
        // don't start execution of binary until debugger is ready
        mStopAtStart = true;
    }

    ////////////////////////////////////////////////////////////////////////////

    // emu thread
    void DebugMgr::QueueEvent(const String& event, const Json& body)
    {
        QDAP(SendEvent(event, body));
    }

    ////////////////////////////////////////////////////////////////////////////

    // dap thread
    bool DebugMgr::OnAttach(const String& binaryFile, const Vector<Json>& listings)
    {
        // on reset load program 
        QEMURESET(
        {
            // on 30th frame after dos is ready, load binary
            gEvents.Queue(Event::Frames{ 30 }, [=]()
            {
                // enable this instance as the session
                gDebugMgrSession = this;

                char path[MAX_PATH];
                strcpy(path, binaryFile.c_str());
                uint16_t start = (uint16_t)-1;
                auto result = QuickLoad(path, &start);
                if (result != 0) QueueEvent("terminated");

                // push first function to stack
                auto realTo = CpuToReal(start);
                auto realFrom = realTo;
                auto realNext = realTo;
                mStack.push_back({ realTo, realFrom, realNext });

                // get current stack
                auto state = CPUGetState();
                mTOS = Address{ state.S };

            }, ET_COMMAND);
        });

        // trigger a reset
        SendHardReset(EmuState.WindowHandle);

        // parse listings
        for (auto& listing : listings)
            mSourceMgr.LoadSource(listing.AsString());
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////

    void DebugMgr::OnBeginBreakpoints(const String& sourceFile)
    {
        auto fileId = mSourceMgr.SourceFileId(sourceFile);
        auto erase_if = [](auto& c, auto pred) 
        {
            auto n = c.size();
            for (auto it = c.begin(); it != c.end(); )
                if (pred(*it)) it = c.erase(it);
                else ++it;
            return n - c.size();
        };
        erase_if(mBreakpoints, [=](const auto &bp) { return bp.second.File == fileId; });
    }

    ////////////////////////////////////////////////////////////////////////////

    bool DebugMgr::OnSetBreakpoint(const String& sourceFile, int lineNo, int id)
    {
        auto line = LineId(lineNo);
        auto address = mSourceMgr.SourceToAddress(sourceFile, line);
        if (address != naddress)
        {
            auto fileId = mSourceMgr.SourceFileId(sourceFile);
            QEMU(mBreakpoints[address] = { id, fileId, line });
        }
        return address != naddress;
    }

    ////////////////////////////////////////////////////////////////////////////

    void DebugMgr::OnConfigurationDone()
    {
        QEMU(mStopAtStart = false);
    }

    ////////////////////////////////////////////////////////////////////////////

    void DebugMgr::Capture(Json& args, const String& command, int requestSeq)
    {
        VCC::IDisplay* GetDisplay();
        uint8_t* bytes;
        int width, height;
        auto result = Screenshot::SnapSurface(GetDisplay(), Screenshot::OPT_RGBA, &bytes, &width, &height);
        if (result != Screenshot::OK)
        {
            delete[] bytes;
            return;
        }
        auto encoded = Base64Encode(bytes, width * height * 4);
        delete[] bytes;
        SendResponse(requestSeq, command, Json::Object{
                { "screenshot", encoded },
                { "width", width },
                { "height", height }
            });
    }

    ////////////////////////////////////////////////////////////////////////////

    void DebugMgr::OnDisconnect()
    {
        QEMU(mStop = false; gDebugMgrSession = nullptr);

        // wait for queue to empty
        while (gEmuCommand.Size())
            Sleep(100);

        // trigger a reset
        SendHardReset(EmuState.WindowHandle);
    }

    ////////////////////////////////////////////////////////////////////////////

    // dap thread
    Json DebugMgr::OnRegisters()
    {
        Json body;
        // read from emu thread, which is currently paused
        auto state = CPUGetState();
        auto& variables = body["variables"].AsArray();
        auto addReg16 = [&variables](const char* name, uint16_t val)
        {
            variables.push_back(Json::Object{
                {"name", name },
                {"value", ToHex(val,4) },
                {"variablesReference", 0 },
                {"type", "uint16" }
                });
        };

        auto addReg8 = [&variables](const char* name, uint8_t val)
        {
            variables.push_back(Json::Object{
                {"name", name },
                {"value", ToHex(val,2) },
                {"variablesReference", 0 },
                {"type", "uint16" }
                });
        };

        addReg8("A", state.A);
        addReg8("B", state.B);
        addReg16("D", ((uint16_t)state.A << 8) + state.B);
        addReg16("X", state.X);
        addReg16("Y", state.Y);
        addReg16("U", state.U);
        addReg16("S", state.S);
        addReg8("DP", state.DP);
        addReg8("CC", state.CC);
        addReg16("PC", state.PC);

        if (state.IsNative6309)
        {
            addReg8("E", state.E);
            addReg8("F", state.F);
            addReg16("W", ((uint16_t)state.E << 8) + state.F);
            addReg16("V", state.V);
            addReg8("MD", state.MD);
        }

        variables.push_back(Json::Object{
            {"name", "Stack" },
            {"value", "{...}" },
            {"variablesReference", 100 }
            });

        return body;
    }

    Json DebugMgr::OnVariables(int ref)
    {
        Json body;
        auto& variables = body["variables"].AsArray();

        if (ref == 100)
        {
            unsigned char SafeMemRead8(unsigned short);
            auto state = CPUGetState();
            auto sp = state.S;
            auto stack = [&](int ofs) { return SafeMemRead8(sp + ofs); };
            auto s16 = [&](int ofs) { return ((uint16_t)stack(ofs) << 8) | stack(ofs + 1); };
            size_t index = mStack.size() - 1;

            auto addRow = [&variables](const String& name, const String& val)
            {
                variables.push_back(Json::Object{
                    {"name", name },
                    {"value", val },
                    {"variablesReference", 0 },
                    {"type", "string" }
                    });
            };

            auto originalSp = sp;
            while (index)
            {
                auto& frame = mStack[--index];
                auto reta = frame.Next & 0xFFFF;

                // walk stack until we found current return address or tos
                int i = 0;
                while (i < 32 && s16(i) != reta && Address{ (uint32_t)sp + i } < mTOS)
                    ++i;
                if (i == 32)
                {
                    // ok didn't find it, program must be using S for data
                    addRow("<aborted>","");
                    return body;
                }

                // how many?
                auto delta = i + 2;
                auto dist = sp - originalSp;
                auto symbol = mSourceMgr.SymbolAt(frame.Dest);
                auto symbolname = symbol.mName;
                auto name = ToHex(sp, 4) + "(+" + ToHex(dist, 2) + ") " + symbolname;
                String row;

                // walk stack and collect bytes
                for (int i = 0; i < delta && i < 16 && Address{ (uint32_t)sp + i } < mTOS; ++i)
                {
                    // if its current return address show that
                    if (reta == s16(i))
                    {
                        row += (i ? ",r" : "r") + ToHex(s16(i), 4);
                        ++i;
                    }
                    // else just show bytes
                    else
                    {
                        row += (i ? "," : "") + ToHex(stack(i), 2);
                    }
                }
                if (sp + i == mTOS)
                {
                    row += i ? ",<TOS>" : "<TOS>";
                    addRow(name, row);
                    return body;
                }
                else
                {
                    addRow(name, row);
                }
                sp += delta;
            }
        }
        return body;
    }

    ////////////////////////////////////////////////////////////////////////////

    size_t DebugMgr::OnStackTraceCount()
    {
        return mStack.size() - 1;
    }
    
    ////////////////////////////////////////////////////////////////////////////

    void DebugMgr::OnStackTrace(int i, String& outName, String& outPath, String& outFunc, int& outLine, int& outColumn)
    {
        size_t index = mStack.size() - i - 1;
        auto& frame = mStack[index];
        auto& prevFrame = index > 0 ? mStack[index - 1] : mStack[index];
        auto& [fileId, lineId] = mSourceMgr.AddressToSource(frame.Source);
        StringView source = mSourceMgr.SourceFileId(fileId);
        auto slash = source.find_last_of("/\\");
        auto dt = (frame.Source - prevFrame.Dest) & 0xFFFF;
        if (slash != StringView::npos)
        {
            auto path = source.substr(0,slash);
            auto fname = source.substr(slash + 1);
            auto symbol = mSourceMgr.SymbolAt(prevFrame.Dest);
            String symName;
            if (symbol.mName.length() == 0 && frame.Source == naddress)
                symName = "__entry__";
            else
                symName = symbol.mName;
            outName = fname;
            outPath = source;
            outFunc = ToHex(frame.Dest, 4) + " " + symName + "(+" + ToHex(dt, dt & 0xFF00 ? 4 : 2) + ")";
            outLine = lineId;
            outColumn = 1;
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    void DebugMgr::OnStep()
    {
        unsigned char SafeMemRead8(unsigned short);
        auto state = CPUGetState();
        auto pc = state.PC;
        auto sp = state.S;
        auto stack = [&](int ofs) { return SafeMemRead8(sp + ofs); };
        auto mem = [&](int ofs) { return SafeMemRead8(pc + ofs); };
        auto off8 = [&]() { return (int8_t)mem(1); };
        auto off16 = [&]() { return (int16_t)((uint16_t)(mem(1) << 8) | mem(2)); };
        static const uint8_t pop4[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
        auto pulsCount = [&](uint8_t operand) { return pop4[operand & 0xF] + 2 * pop4[(operand >> 4) & 0x7]; };
        auto opcode = mem(0);
        Address branchTarget = naddress;

        // calculate a branch target, rts/branch/puls
        if (opcode >= 0x20 && opcode <= 0x2F) // conditional
            branchTarget = Address((uint16_t)(pc + 2) + off8());
        else if (opcode == 0x10)
        {
            pc++;
            opcode = mem(0);
            if (opcode >= 0x22 && opcode <= 0x2F) // long conditional
                branchTarget = Address((uint16_t)(pc + 3) + off16());
        }
        else if (opcode == 0x39) // rts
            branchTarget = (Address)(((uint16_t)stack(0) << 8) | stack(1));
        else if (opcode == 0x35 || opcode == 0x37) // pul
        {
            pc++;
            auto operand = mem(0);
            if (operand & 0x80) // pul pc?
            {
                auto at = pulsCount(operand);
                branchTarget = (Address)(((uint16_t)stack(at) << 8) | stack(at + 1));
            }
        }

        // breakpoint on next line or branch target
        auto address = mSourceMgr.NextSourceToAddress(mCurrentFile, mCurrentLine);
        auto address2 = branchTarget != naddress ? CpuToReal(branchTarget) : naddress;

        // stop at address or address2
        QEMU(mStop = false; mStopAt = address; mStopAt2 = address2);
    }

    ////////////////////////////////////////////////////////////////////////////

    void DebugMgr::OnStepIn()
    {
        // just run one instruction
        QEMU(mStop = false; QEMU(mStop = true));
    }

    ////////////////////////////////////////////////////////////////////////////

    void DebugMgr::OnStepOut()
    {
        size_t index = mStack.size() - 1;
        auto& frame = mStack[index - 1];
        auto reta = frame.Next;

        QEMU(mStop = false; mStopAt = reta, mStopRts = true);
    }

    ////////////////////////////////////////////////////////////////////////////

    void DebugMgr::OnContinue()
    {
        QEMU(mStop = false);
    }

    ////////////////////////////////////////////////////////////////////////////

    DebugMgr::StringPair DebugMgr::OnEvaluate(const String& context, int frameId, const String& expression)
    {
        using SV = StringView;
        unsigned char SafeMemRead8(unsigned short);
        auto state = []() { return CPUGetState(); };
        auto mem8 = [&](int ofs) { return SafeMemRead8(ofs); };
        auto mem16 = [&](int ofs) { return (mem8(ofs) << 8) + mem8(ofs + 1); };
        auto mem32 = [&](int ofs) { return (mem16(ofs) << 16) + mem16(ofs + 2); };

        auto reg = [&](SV& text) -> String
        {
            switch (text[0])
            {
                case 'a': return ToHex(state().A,2);
                case 'b': return ToHex(state().B,2);
                case 'd': return ToHex((uint16_t)(state().A << 8) | state().B,4);
                case 'x': return ToHex(state().X,4);
                case 'y': return ToHex(state().Y,4);
                case 'u': return ToHex(state().U,4);
                case 's': return ToHex(state().S,4);
            }
            return "";
        };

        auto getSize = [&](SV& text)
        {
            switch (text[0])
            {
                case 'b': return 1;
                case 'w': return 2;
                case 'd': return 4;
            }
            return 0;
        };

        if (context == "hover" || context == "watch" || context.empty())
        {
            SV sv(expression);
            if (sv.empty()) return {};
            if (sv.length() == 1) return StringPair{ String(sv), reg(sv) };
            if (sv[1] == ' ') sv.remove_prefix(2);
            auto symbol = mSourceMgr.SymbolInfo(String(sv));

            // get size and address
            int siz = symbol.mSize;
            auto address = symbol.mAddr;

            // override size?
            siz = (sv[1] == ' ') ? getSize(sv) : siz;

            switch (siz)
            {
                case 0: return { ToHex(address, 6), "address" };
                case 1: return { ToHex(mem8(address),2), "byte" };
                case 2: return { ToHex(mem16(address),4), "word" };
                case 4: return { ToHex(mem32(address),8), "dword" };
            }
        }
        return {};
    }

    ////////////////////////////////////////////////////////////////////////////

    Json DebugMgr::OnRepl(const String& command, Json& args)
    {
        auto startsWith = [&](auto text) { return _strcmpi(command.data(), text) != 0; };
        
        if (startsWith("/dump-symbols"))
        {
            auto arg = TrimWs(command.substr(13));
            mSourceMgr.DumpSymbolFile(arg);
            return Json::Object{ { "result", "Done" } };
        }
        return {};
    }
}