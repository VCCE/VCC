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

#include "SourceMgr.h"
#include "StringUtils.h"
#include "Json.h"

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <string>

#include <Windows.h>

#define CFG_DEBUG_SYMBOLS true

namespace VCC
{
    namespace
    {
        // file limit
        constexpr auto fileIdBits = 16;
        constexpr auto fileIdMask = (1 << fileIdBits) - 1;

        auto log = [](const char* fmt, auto ...args)
        {
#if CFG_DEBUG_SYMBOLS
            static char tmp[65536];
            sprintf(tmp, fmt, args...);
            OutputDebugString(tmp);
#endif
        };
    }

    SourceMgr::String SourceMgr::mEmptyString{};
    SourceMgr::Symbol SourceMgr::mEmptySymbol{};

    //////////////////////////////////////////////////////////////////////////

    SourceMgr::FileId SourceMgr::InsertFile(const StringView& fileName, bool create)
    {
        auto found = std::find(mFiles.begin(), mFiles.end(), fileName);
        if (found != mFiles.end())
            return (FileId)std::distance(mFiles.begin(), found);

        if (create)
        {
            mFiles.push_back(String(fileName));
            auto id = mFiles.size() - 1;
            auto idn = (FileId::Type)(id);
            if (id != idn || id >= fileIdMask) throw std::runtime_error("Too many includes");
            return FileId(idn);
        }
        return nfile;
    };

    //////////////////////////////////////////////////////////////////////////

    SourceMgr::FileId SourceMgr::SourceFileId(const String& fileName)
    {
        auto split = JoinPath(SplitPath(fileName));
        auto file = InsertFile(split);
        return file;
    }

    //////////////////////////////////////////////////////////////////////////

    const SourceMgr::Symbol& SourceMgr::SymbolAt(Address address)
    {
        auto found = std::upper_bound(mSymbols.begin(), mSymbols.end(), address, [](auto& a, auto& b) { return a < b.mAddr; });
        if (found == mSymbols.begin())
            return mEmptySymbol;
        return *(found-1);
    }

    //////////////////////////////////////////////////////////////////////////

    const SourceMgr::Symbol& SourceMgr::SymbolInfo(const String& symbol)
    {
        auto address = SymbolAddress(symbol);
        return SymbolAt(address);
    }

    //////////////////////////////////////////////////////////////////////////

    SourceMgr::Address SourceMgr::SymbolAddress(const String& symbol)
    {
        auto found = mSymbolTable.find(symbol);
        if (found == mSymbolTable.end())
            return naddress;
        return found->second;
    }

    //////////////////////////////////////////////////////////////////////////

    const SourceMgr::String& SourceMgr::SourceFileId(FileId fileId)
    {
        return mFiles.at(fileId);
    }

    //////////////////////////////////////////////////////////////////////////

    void SourceMgr::LoadSource(const String& fileName)
    {
        LoadLwasmSource(fileName);
        std::sort(mSymbols.begin(), mSymbols.end(), [](auto& a, auto& b) { return a.mAddr < b.mAddr; });
    }

    //////////////////////////////////////////////////////////////////////////
    // Loads the .lst format from lwasm
    //
    void SourceMgr::LoadLwasmSource(const String& fileName)
    {
        using SV = StringView;

        // lwasm .lst format
        auto constexpr COL_OPCODES = 5;
        auto constexpr COL_FNAME = 22+1;
        auto constexpr COL_LINENO = 42;

        // start at 56, may increase
        int codeCol = 56;
        bool opt = false;
        auto lines = LoadTextFile(fileName);
        LineId currentLine{};
        FileId currentFile{};
        String currentPath{};
        LineId lastLine{};
        FileId currentSourceFile{};
        String currentSourcePath{};
        LineId currentSourceLine{};
        Vector<Pair<String,String>> pathStack;

        // mmu state
        size_t mmu[16];
        for (size_t i = 0; i < 8; ++i)
        {
            mmu[i] = 0x38 + i;
            mmu[i + 8] = 0x38 + i;
        }

        // helpers
        auto ws = [](char c) { return c == ' ' || c == '\t'; };
        auto eol = [](char c) { return c == '\r' || c == '\n'; };
        auto hex = [](char c) { return isdigit(c) || c >= 'A' && c <= 'F'; };
        auto nibble = [](char c) { return isdigit(c) ? (c - '0') : (c - 'A' + 10); };
        auto startsWith = [](const SV& sv, const char* s) { return sv.rfind(s, 0) == 0; };
        auto skipWs = [&](const SV& text) { SV sv(text); while (sv.size() && ws(sv[0])) sv.remove_prefix(1); return sv; };
        auto trimWs = [](const SV& text) { return TrimWs(text); };

        // return byte
        auto fromHex = [&](const SV& hex)
        {
            return ((unsigned)nibble(hex[0]) << 4) | nibble(hex[1]);
        };

        // parse address follows
        auto parseAddress = [&](const SV& line)
        {
            if (line.size() < 4) return naddress;
            if (!hex(line[0])) return naddress;
            if (!hex(line[1])) return naddress;
            if (!hex(line[2])) return naddress;
            if (!hex(line[3])) return naddress;
            return Address{ (fromHex(line) << 8) | fromHex(line.substr(2)) };
        };

        // parse an address at start followed by space
        auto parseAddressSpace = [&](const SV& line)
        {
            auto address = parseAddress(line);
            if (address == naddress) return naddress;

            // check for space following (to skip 's')
            if (line[4] != ' ')
                return naddress;

            // hw register?
            if (address >= 0xFF90) return address;

            // return real address
            auto page = address >> 13;
            return Address{ (address & 0x1FFF) + (mmu[page] << 13) };
        };

        // read string until ws, starting at pos
        auto parseStringAt = [&](const SV& line, size_t pos)
        {
            SV sv(line);
            sv.remove_prefix(std::min(pos,sv.size()));
            size_t i = 0;
            while (i < sv.size() && !ws(sv[i]) && !eol(sv[i])) ++i;
            return sv.substr(0, i);
        };

        // read symbol
        auto parseSymbol = [&](auto& line) { return parseStringAt(line, codeCol); };
        auto parseOpcodes = [&](auto& line) { return parseStringAt(line, COL_OPCODES); };

        auto parseInstruction = [&](auto& line)
        {
            SV sv(line);
            sv.remove_prefix(std::min((size_t)codeCol, sv.size()));
            while (sv.size() && sv.front() != ' ') sv.remove_prefix(1);
            sv = trimWs(sv);
            return parseStringAt(sv, 0);
        };

        // read the file inside ()
        auto parseFile = [&](const SV& line)
        {
            SV sv(line);
            sv.remove_prefix(std::min((size_t)COL_FNAME, sv.size()));
            auto close = sv.find_first_of(')');
            if (close == SV::npos) return StringView{};
            return trimWs(sv.substr(0, close));
        };

        // just parse number at str
        auto parseNumber = [](const SV& str)
        {
            auto space = str.find_first_of(' ');
            if (space == SV::npos) space = str.size();
            auto num = str.substr(0, space);
            return (LineId)ParseInt(num);
        };

        // read the line number after :
        auto parseLineNumber = [=](const SV& line)
        {
            SV sv(line);
            sv.remove_prefix(COL_LINENO);
            return parseNumber(sv);
        };

        // read the line number after #line
        auto parsePpLine = [&](const SV& line)
        {
            SV sv(line);
            sv.remove_prefix(std::min((size_t)codeCol + 6, sv.size()));
            return parseNumber(sv);
        };

        // just skip next symbol
        auto skipSymbol = [&](const SV& text)
        {
            SV sv(text);
            auto sym = parseStringAt(sv, 0);
            sv.remove_prefix(sym.length());
            return trimWs(sv);
        };

        // read the preprocessor file after #line num <file>
        auto parsePpFile = [&](const SV& line)
        {
            SV sv(line);
            sv.remove_prefix(codeCol + 6); // skip #line
            sv = skipSymbol(sv); // skip num
            auto fn = parseStringAt(sv, 0);
            if (!fn.length()) return fn;
            return fn.substr(1, fn.length() - 2); // strip quotes
        };

        auto isComment = [&](const SV& line)
        {
            SV sv(line);
            sv.remove_prefix(std::min((size_t)codeCol, sv.size()));
            sv = trimWs(sv);
            return !sv.size() || sv.front() == '*' || sv.front() == ';';
        };

        auto parseStrLen = [&](const SV& line)
        {
            return 0;
        };

        mPath = SplitPath(fileName);
        currentPath = JoinPath(mPath);
        mPath.pop_back();

        // establish base path
        mBasePath = JoinPath(mPath);

        auto basePath = mBasePath;
        auto sourceBasePath = mBasePath;

        // push current to stack
        auto push = [&](const String& path)
        {
            log("%*s push path: %s\n", pathStack.size() * 2, "", path.c_str());
            pathStack.push_back({ path, basePath });
        };

        // pop stack to current
        auto pop = [&]()
        {
            log("%*s pop path: %s\n", pathStack.size() * 2, "", currentPath.c_str());
            auto [path, base] = *pathStack.rbegin();
            currentPath = path;
            basePath = base;
            currentFile = InsertFile(currentPath, false);
            pathStack.pop_back();
            log("%*s new path: %s\n", pathStack.size() * 2, "", currentPath.c_str());
        };

        bool first = true;
        bool useSourceLines = false;
        int inMacro = 0;
        for (auto& raw : lines)
        {
            SV textLine(raw);

            // skip over comment lines
            if (isComment(textLine))
            {
                lastLine = (LineId)(lastLine + 1);
                currentSourceLine = (LineId)(currentSourceLine + 1);
                continue;
            }

            auto address = parseAddressSpace(textLine);
            auto symbol = String(trimWs(parseSymbol(textLine)));

            // read filename and check it still matches, if not then
            // must have reached end of file so pop current off stack
            auto fileStr = parseFile(textLine);
            auto isMacro = fileStr.find_first_of('.') == String::npos;

            if (isMacro)
            {
                lastLine = LineId(lastLine - 1);
            }

            if (first)
            {
                currentPath = JoinPath(basePath, { String(fileStr) });
                currentFile = InsertFile(currentPath, true);
                first = false;
                auto split = SplitPath(currentPath);
                split.pop_back();
                basePath = JoinPath(split);
                push(currentPath);
            }
            // is a file (not macro)
            else if (fileStr.length() > 0 && !isMacro)
            {
                while (true)
                {
                    // if it doesn't match, pop file stack
                    auto fileMatch = currentPath.rfind(fileStr);
                    if (fileMatch != String::npos) break;
                    pop();
                    lastLine = parseLineNumber(textLine);
                }
            }

            auto instr = trimWs(parseInstruction(textLine));
            auto instrLwr = ToLower(String(instr));

            if (instrLwr == "macro")
            {
                ++inMacro;
                log("macro begin %d\n",inMacro);
            }
            else if (instrLwr == "endm")
            {
                log("macro end %d\n", inMacro);
                --inMacro;
            }

            if (!opt && !inMacro)
            {
                if (instrLwr == "opt")
                {
                    // if timings then add the new column
                    auto pos = textLine.find(instr);
                    auto remain = textLine.substr(pos);
                    if (remain.find("ct"))
                    {
                        log("opt timings\n");
                        codeCol += 8;
                        opt = true;
                    }
                }
            }

            if (instrLwr == "include")
            {
                // follow include
                auto pos = textLine.find(instr);
                auto remain = textLine.substr(pos);
                auto arg = parseStringAt(remain,8);
                auto newFile = SplitPath(String(arg));
                push(currentPath);
                currentPath = JoinPath(basePath, newFile);
                currentFile = InsertFile(currentPath, true);
                newFile.pop_back();
                basePath = JoinPath(basePath, newFile);
                log("%*sincl path: %s\n", pathStack.size() * 2, "", currentPath.c_str());
                lastLine = LineId(1);
                useSourceLines = false;

                // skip to next line
                continue;
            }

            if (symbol == "#line")
            {
                // if found a source line, use this
                useSourceLines = true;
                auto lineNo = parsePpLine(textLine);
                auto fileStr = parsePpFile(textLine);

                // if file switch, otherwise same file
                if (fileStr.length() > 0)
                {
                    auto newFile = SplitPath(String(fileStr));
                    currentSourcePath = JoinPath(sourceBasePath, newFile);
                    currentSourceFile = InsertFile(currentSourcePath, true);
                }
                currentSourceLine = lineNo;

                // skip to next line
                continue;
            }

            // fetch address for line
            if (address != naddress)
            {
                // symbol at this address record it
                if (symbol.length() > 0)
                {
                    int size = 0;
                    if (instr.length() > 0)
                    {
                        auto pos = textLine.find(instr);
                        if (pos != StringView::npos)
                        {
                            auto remain = trimWs(textLine.substr(pos + 1));
                            if (instrLwr == "fcb") size = 1;
                            else if (instrLwr == "fdb") size = 2;
                            else if (instrLwr == "fqb") size = 4;
                            else if (instrLwr == "fcc") size = parseStrLen(remain);
                            else if (instrLwr == "fcn") size = parseStrLen(remain);
                            else if (instrLwr == "fcs") size = parseStrLen(remain);
                        }
                    }

                    InsertSymbol(symbol, address, size);
                }

                auto opcodes = parseOpcodes(textLine);

                // handle mmu write
                if (address >= 0xFFA0 && address <= 0xFFAF)
                {
                    while (opcodes.length() > 0)
                    {
                        auto value = fromHex(opcodes.substr(0, 2));
                        opcodes.remove_prefix(2);
                        mmu[address - 0xFFA0] = value;
                        log("mmu: %04X - %02X\n", address, value);
                    }
                    continue;
                }

                // has opcodes at this address, record the line
                if (opcodes.length() > 0)
                {
                    if (useSourceLines)
                        InsertLine(address, currentSourceFile, currentSourceLine);
                    else if (isMacro)
                        InsertLine(address, currentFile, lastLine);
                    else
                    {
                        auto lineNo = parseLineNumber(textLine);
                        InsertLine(address, currentFile, lineNo);
                        lastLine = lineNo;
                    }
                }
            }

            lastLine = (LineId)(lastLine + 1);
            currentSourceLine = (LineId)(currentSourceLine + 1);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    SourceMgr::FileLineKey SourceMgr::MakeKey(FileId file, LineId line)
    {
        return FileLineKey{ ((size_t)file << (32 - fileIdBits)) | (size_t)line };
    }

    //////////////////////////////////////////////////////////////////////////

    void SourceMgr::InsertLine(Address address, FileId file, LineId line)
    {
        mInstructions.push_back({ address, line, file });
        FileLineKey key = MakeKey(file, line);
        mLineToAddr[key] = address;
        log("line: %06x %s(%d)\n", address, mFiles[file].c_str(), line);
    }

    //////////////////////////////////////////////////////////////////////////

    void SourceMgr::InsertSymbol(const StringView& symbol, Address address, int size)
    {
        StringView sv(symbol);
        while (sv.back() == ':') sv.remove_suffix(1);
        String sym(sv);
        mSymbols.push_back({ sym, address, (uint16_t)size });
        mSymbolTable.insert({ sym, address });
        log("symbol: %06X %s\n", address, sym.c_str());
    }

    //////////////////////////////////////////////////////////////////////////

    SourceMgr::Pair<SourceMgr::FileId, SourceMgr::LineId> SourceMgr::AddressToSource(Address address)
    {
        auto it = std::upper_bound(mInstructions.begin(), mInstructions.end(), address,
            [](Address addr, const Instr& o) { return addr < o.mAddr; });
        if (it != mInstructions.begin()) --it;
        const auto& o = *it;
        return { o.mFile, o.mLine };
    }

    //////////////////////////////////////////////////////////////////////////

    SourceMgr::Address SourceMgr::SourceToAddress(const String& fileName, LineId line, int maxLines)
    {
        auto split = JoinPath(SplitPath(fileName));
        auto file = InsertFile(split);
        if (file == nfile) return naddress;
        return SourceToAddress(file, line, maxLines);
    }

    //////////////////////////////////////////////////////////////////////////

    SourceMgr::Address SourceMgr::NextSourceToAddress(FileId file, LineId line)
    {
        auto key = MakeKey(file, line);

        auto it = mLineToAddr.find(key);
        if (it == mLineToAddr.end()) return naddress;

        // walk to next
        ++it;
        if (it == mLineToAddr.end()) return naddress;

        return it->second;
    }

    SourceMgr::Address SourceMgr::SourceToAddress(FileId file, LineId line, int maxLines)
    {
        auto key = MakeKey(file, line);

        auto search = [&](FileLineKey key)
        {
            auto it = mLineToAddr.find(key);
            if (it != mLineToAddr.end())
                return it->second;
            return naddress;
        };

        auto found = search(key);
        if (found != naddress) return found;

        for (int d = 1; d <= maxLines; ++d)
        {
            key = MakeKey(file, LineId(line + d));
            auto found = search(key);
            if (found != naddress) return found;
        }

        for (int d = 1; d <= maxLines && (int)line >= d; ++d)
        {
            key = MakeKey(file, LineId(line - d));
            auto found = search(key);
            if (found != naddress) return found;
        }

        return naddress;
    }

    void SourceMgr::DumpSymbolFile(const StringView& path)
    {
        Json symbolFile;
        symbolFile["ver"] = 1;
        auto& files = symbolFile["files"].AsArray();
        auto& symbols = symbolFile["symbols"].AsArray();
        auto& instructions = symbolFile["instructions"].AsArray();

        for (auto& fil : mFiles)
            files.push_back(Json(fil));

        for (auto& sym : mSymbols)
        {
            if (sym.mSize)
            {
                symbols.push_back(Json::Object{
                    { "name", sym.mName },
                    { "addr", (long)sym.mAddr },
                    { "size", (long)sym.mSize }
                    });
            }
            else
            {
                symbols.push_back(Json::Object{
                    { "name", sym.mName },
                    { "addr", (long)sym.mAddr },
                    });
            }
        }

        for (auto& ins : mInstructions)
        {
            instructions.push_back(Json::Object{
                { "addr", (long)ins.mAddr },
                { "file", (long)ins.mFile },
                { "line", (long)ins.mLine }
                });
        }

        auto raw = symbolFile.ToString(Json::TS_NEWLINES|Json::TS_INDENT);

        String p(path);
        if (p.find_first_of('/') == String::npos && p.find_first_of('\\') == String::npos)
        {
            // put in base path if no path found
            p = JoinPath(mBasePath, SplitPath(p));
        }

        std::ofstream file(p);
        if (!file) return;
        file << raw;
        file.close();
    }
}
