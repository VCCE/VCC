#pragma once

#include <cstdint>

//
// Hook into debugger interface. A session is active when debugger is attached
// this global is set via the emulator thread so as not to conflict.
//
#define IDEBUG(...) do { if (VCC::IDebug::gDebugSession) VCC::IDebug::gDebugSession->__VA_ARGS__; } while (false)

namespace VCC
{
    struct IDebug
    {
        //
        // the session
        //
        static IDebug* gDebugSession;

        virtual void CheckBreakpoints(uint16_t pc) {}
        virtual void Call(uint16_t to, uint16_t from, uint16_t next) {}
        virtual void Return(uint16_t to, uint16_t from) {}
        virtual void SetStack(uint16_t ns, uint16_t os) {}
        virtual void Cwai(uint16_t& pc, uint16_t& x, uint16_t& y, uint16_t& u, uint16_t& s, uint8_t& a, uint8_t& b) {}
    };
}
