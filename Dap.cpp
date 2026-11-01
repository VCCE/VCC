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


#include "Dap.h"
#include "StringUtils.h"

#include <memory>
#include <stdexcept>
#include <Windows.h>

#define CFG_DEBUG_DAP true

namespace VCC
{
    namespace
    {
        auto log = [](const char* fmt, auto ...args)
        {
#if CFG_DEBUG_DAP
            static char tmp[65536];
            sprintf(tmp, fmt, args...);
            OutputDebugString(tmp);
#endif
        };
    }


    Dap::Dap(Socket client) : mClient(std::move(client)), mSeq(0), mBpIndex(0)
    {
    }

    Dap::~Dap()
    {
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Main(int port, AtmoicBool& stop, Function<DapPtr(Socket)> create)
    {
        Socket::Init();
        Socket server;
        if (server.Listen("127.0.0.1", port) != Socket::OK)
            return;

        while (!stop)
        {
            // wait for connection
            Socket client;
            auto r = server.Accept(client, 100);
            if (r == Socket::ERR_TIMEDOUT) continue;
            if (r != Socket::OK) break;

            // create dap session
            DapPtr dap = create(std::move(client));
            dap->Run(stop);
        }

        server.Close();
        Socket::Shutdown();
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Initialize(Json& args, const String& command, int requestSeq)
    {
        OnInitialize();
        SendResponse(requestSeq, command, Json::Object{
            { "supportsConfigurationDoneRequest", true },
            { "supportsEvaluateForHovers", true },
            { "supportsSetVariable", false },
            { "supportsStepBack", false }
            });
        SendEvent("initialized");
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::SetBreakpoints(Json& args, const String& command, int requestSeq)
    {
        auto& arguments = args["arguments"];
        auto& source = arguments["source"];
        auto& breakpoints = arguments["breakpoints"];
        auto& sourcePath = source["path"].AsString();
        OnBeginBreakpoints(sourcePath);
        
        for (auto& bp : breakpoints.AsArray())
        {
            auto id = ++mBpIndex;
            bool success = OnSetBreakpoint(sourcePath, bp["line"].AsInt(), id);
            bp["verified"] = (bool)success;
            bp["id"] = id;
            bp["source"] = source;
            if (!success)
                bp["message"] = "No code near this line found";
        }
        SendResponse(requestSeq, command, Json::Object{{ "breakpoints", breakpoints }});
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::ConfigurationDone(Json& args, const String& command, int requestSeq)
    {
        OnConfigurationDone();
        SendResponse(requestSeq, command);
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Attach(Json& args, const String& command, int requestSeq)
    {
        SendResponse(requestSeq, command);
        OnAttach(args["arguments"]["program"].AsString(), args["arguments"]["listings"].AsArray());
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Disconnect(Json& args, const String& command, int requestSeq)
    {
        SendResponse(requestSeq, command);
        OnDisconnect();
        SendEvent("terminated");
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Threads(Json& args, const String& command, int requestSeq)
    {
        SendResponse(requestSeq, command, Json::Object{
            { "threads", Json::Array{
                Json::Object{
                    { "id", 1 },
                    { "name", "main" }
                }}
            }
            });
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::StackTrace(Json& args, const String& command, int requestSeq)
    { 
        Json body;
        auto& frames = body["stackFrames"].AsArray();
        auto depth = OnStackTraceCount();
        body["totalFrames"] = (long)depth;
        for (size_t i = 0; i < depth; ++i)
        {
            String name, func, path;
            int line, column;
            OnStackTrace(i, name, path, func, line, column);

            Json frame;
            frame["id"] = (int)(i + 1);
            frame["name"] = func;
            frame["line"] = line;
            frame["column"] = column;
            frame["source"] = Json::Object{
                { "name", name },
                { "path", path }
            };
            frames.push_back(frame);
        }
        SendResponse(requestSeq, command, body);
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Scopes(Json& args, const String& command, int requestSeq)
    {
        SendResponse(requestSeq, command, Json::Object{
            { "scopes", Json::Array{
                Json::Object{
                    { "name", "Locals" },
                    { "presentationHint", "locals" },
                    { "variablesReference", 1 },
                    { "expensive", false },
                },
                Json::Object{
                    { "name", "Registers" },
                    { "presentationHint", "registers" },
                    { "variablesReference", 2 },
                    { "expensive", false },
                }}
            } });
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Variables(Json& args, const String& command, int requestSeq)
    {
        auto ref = args["arguments"]["variablesReference"].AsInt();
        if (ref == 2)
        {
            auto body = OnRegisters();
            SendResponse(requestSeq, command, body);
        }
        else if (ref)
        {
            auto body = OnVariables(ref);
            SendResponse(requestSeq, command, body);
        }
        else SendResponse(requestSeq, command);
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Next(Json& args, const String& command, int requestSeq)
    {
        OnStep();
        SendResponse(requestSeq, command);
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::StepIn(Json& args, const String& command, int requestSeq)
    {
        OnStepIn();
        SendResponse(requestSeq, command);
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::StepOut(Json& args, const String& command, int requestSeq)
    {
        OnStepOut();
        SendResponse(requestSeq, command);
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Continue(Json& args, const String& command, int requestSeq)
    {
        OnContinue();
        SendResponse(requestSeq, command);
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Evaluate(Json& args, const String& command, int requestSeq)
    {
        auto& arguments = args["arguments"];
        auto& context = arguments["context"].AsString();
        auto& expression = arguments["expression"].AsString();

        if (context == "repl")
        {
            SendResponse(requestSeq, command, OnRepl(expression, args));
            return;
        }

        auto frameId = arguments["frameId"].AsInt();
        auto [result, type] = OnEvaluate(context, frameId, expression);
        SendResponse(requestSeq, command, Json::Object{
            { "result", result },
            { "variablesReference", 0 },
            { "type", type }
        });
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::InitFunctions()
    {
        AddFunction("initialize", [=](auto ...args) { Initialize(args...); });
        AddFunction("setBreakpoints", [=](auto ...args) { SetBreakpoints(args...); });
        AddFunction("configurationDone", [=](auto ...args) { ConfigurationDone(args...); });
        AddFunction("attach", [=](auto ...args) { Attach(args...); });
        AddFunction("disconnect", [=](auto ...args) { Disconnect(args...); });
        AddFunction("threads", [=](auto ...args) { Threads(args...); });
        AddFunction("stackTrace", [=](auto ...args) { StackTrace(args...); });
        AddFunction("scopes", [=](auto ...args) { Scopes(args...); });
        AddFunction("variables", [=](auto ...args) { Variables(args...); });
        AddFunction("next", [=](auto ...args) { Next(args...); });
        AddFunction("stepIn", [=](auto ...args) { StepIn(args...); });
        AddFunction("stepOut", [=](auto ...args) { StepOut(args...); });
        AddFunction("continue", [=](auto ...args) { Continue(args...); });
        AddFunction("evaluate", [=](auto ...args) { Evaluate(args...); });
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::AddFunction(const String& command, DispatchFunc func)
    {
        mDispatch[command] = func;
    }

    ////////////////////////////////////////////////////////////////////////////

    bool Dap::DataWaiting()
    {
        return mClient.Poll(10) == Socket::OK;
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::Run(AtmoicBool& stop)
    {
        InitFunctions();
        while (!stop)
        {
            if (DataWaiting())
            {
                auto msg = ReadMessage();
                log("recv: %s\n", msg.ToString().c_str());
                auto& command = msg["command"].AsString();
                auto requestSeq = msg["seq"].AsInt();
                if (command.length() == 0) break;
                if (!mClient.IsConnected()) break;
                auto dispatch = mDispatch.find(command);
                if (dispatch != mDispatch.end())
                    dispatch->second(msg, command, requestSeq);
                if (command == "disconnect") break;
            }
            else OnIdle();
        }
        // make sure socket has time to send data before closing
        Sleep(500);
    }

    ////////////////////////////////////////////////////////////////////////////

    Json Dap::ReadMessage()
    {
        try
        {
            const size_t bufferLen = 1024;
            char buffer[bufferLen];

            // read the html style header
            String header;
            size_t blanks;
            while (true)
            {
                int bytes = 0;
                if (mClient.Recv(buffer, 1, &bytes) != Socket::OK)
                    return Json::Empty;

                header.append(buffer, bytes);
                if (header.size() < 4) continue;
                blanks = header.find("\r\n\r\n");
                if (blanks != String::npos) break;
            }

            // find content length
            StringView svh(header);
            int length = 0;
            auto pos = svh.find("Content-Length:");
            const auto width = 15;
            if (pos != String::npos)
                length = ParseInt(TrimWs(svh.substr(pos + width, blanks - (pos + width))));

            // skip past the header & blanks
            svh.remove_prefix(blanks + 4);

            // read the body
            String body;
            body.reserve(length);

            // the remaining
            while ((int)body.size() < length)
            {
                auto remain = length - body.size();
                auto read = std::min(bufferLen, remain);
                int bytes = 0;
                if (mClient.Recv(buffer, read, &bytes) != Socket::OK)
                    return Json::Empty;
                body.append(buffer, bytes);
            }

            return Json::Parse(body);
        }
        catch (std::runtime_error&e)
        {
            (void)e;
        }
        return {};
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::SendEvent(const String& event, const Json& body)
    {
        auto packet = Json::Object{
            { "seq", mSeq++ },
            { "type", "event" },
            { "event", event },
            { "success", true }
        };
        if (!body.IsEmpty()) 
            packet["body"] = body;
        WriteMessage(packet);
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::SendResponse(int reqSeq, const String& command, const Json& body)
    {
        auto packet = Json::Object{
            { "seq", mSeq++ },
            { "type", "response" },
            { "request_seq", reqSeq },
            { "command", command },
            { "success", true }
        };
        if (!body.IsEmpty()) 
            packet["body"] = body;
        WriteMessage(packet);
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::SendError(int reqSeq, const String& command, const String& message)
    {
        auto packet = Json::Object{
            { "seq", mSeq++ },
            { "type", "response" },
            { "request_seq", reqSeq },
            { "command", command },
            { "success", false },
            { "message", message }
        };
        WriteMessage(packet);
    }

    ////////////////////////////////////////////////////////////////////////////

    void Dap::WriteMessage(const Json& msg)
    {
        auto json = msg.ToString();
        String packet = "Content-Length: " + std::to_string(json.size()) + "\r\n\r\n" + json;
        size_t sent = 0;
        log("sent: %s\n", json.c_str());
        while (sent < packet.size())
        {
            int bytes = 0;
            if (mClient.Send(packet.data() + sent, packet.size() - sent, &bytes) != Socket::OK)
                return;
            sent += bytes;
        }
    }
}
