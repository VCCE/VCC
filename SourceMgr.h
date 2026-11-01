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
#include "Strong.h"
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>

namespace VCC
{
    struct SourceMgr
    {
        using String = std::string;
        using StringView = std::string_view;
        template <typename T> using Vector = std::vector<T>;
        template <typename ...T> using Pair = std::pair<T...>;
        template <typename ...T> using Map = std::map<T...>;
        template <typename ...T> using UnorderedMap = std::unordered_map<T...>;
        struct AddressTag {};
        struct FileIdTag {};
        struct LineIdTag {};
        struct FileLineKeyTag {};
        using Address = Strong<uint32_t, AddressTag>;
        using FileId = Strong<uint16_t, FileIdTag>;
        using LineId = Strong<uint16_t, LineIdTag>;
        using FileLineKey = Strong<uint32_t, FileLineKeyTag>;
        using LineToAddressMap = Map<FileLineKey, Address>;
        using SymbolMap = UnorderedMap<String, Address>;

        static constexpr Address naddress = (Address)-1;
        static constexpr LineId nline = (LineId)-1;
        static constexpr FileId nfile = (FileId)-1;

        struct Instr
        {
            Address mAddr;      // address at line
            LineId mLine;       // line number
            FileId mFile;       // file index to mFiles
        };

        struct Symbol
        {
            String mName;       // name of symbol
            Address mAddr;      // address of symbol
            uint16_t mSize{};   // width of symbol
        };

        struct Stack
        {
            Address mCaller;    // address of caller
            Address mFrame;     // stack frame
        };

        //
        // load a source listing (lwasm)
        //
        void LoadSource(const String& fileName);

        //
        // translate real address into source file location
        //
        Pair<FileId, LineId> AddressToSource(Address address);

        //
        // translate source file location into real address
        //
        Address SourceToAddress(const String& fileName, LineId line, int maxLines = 2);
        Address SourceToAddress(FileId file, LineId line, int maxLines = 2);
        Address NextSourceToAddress(FileId file, LineId line);

        //
        // resolve filename into path or fileid
        //
        FileId SourceFileId(const String& fileName);
        const String& SourceFileId(FileId fileId);

        //
        // return symbol at given address
        //
        const Symbol& SymbolAt(Address address);
        Address SymbolAddress(const String& symbol);
        const Symbol& SymbolInfo(const String& symbol);


        //
        // write symbol file
        //
        void DumpSymbolFile(const StringView& path);

    private:

        String mBasePath;                       // project base path
        Vector<String> mPath;                   // path to source listing
        Vector<String> mFiles;                  // file table
        Vector<Symbol> mSymbols;                // symbol table
        SymbolMap mSymbolTable;                 // name ordered table
        Vector<Instr> mInstructions;            // address to line translation (by address)
        LineToAddressMap mLineToAddr;           // line to address translation (by file/line)
        static String mEmptyString;             // the empty symbol name
        static Symbol mEmptySymbol;             // the empty symbol

        //
        // loader of lwasm source assembly listing
        //
        void LoadLwasmSource(const String& fileName);

        //
        // insert line information from listing
        //
        void InsertLine(Address address, FileId file, LineId line);

        //
        // insert symbol information from listing
        //
        void InsertSymbol(const StringView& symbol, Address address, int size);

        //
        // insert or resolve a filename to file id
        //
        FileId InsertFile(const StringView& fileName, bool create = false);
        FileLineKey MakeKey(FileId file, LineId line);
    };
}