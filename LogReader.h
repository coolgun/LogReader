#pragma once
#include <windows.h>
#include "Utilities.h"

namespace log_test
{
    using fun = void(*)(const char* buf, size_t bufsize);
    class CLogReader final
    {
        class CTextFile* text_file{};
        class CSimpleRegexp* reg_exp{};

        CLogReader(CLogReader&) = delete;
        CLogReader(CLogReader&&) = delete;

        CLogReader& operator==(CLogReader&) = delete;
        CLogReader& operator==(CLogReader&&) = delete;

    public:
        CLogReader(const char* filter = nullptr);
        ~CLogReader();

        // Opening a file from disk
        bool Open(const char* file_name);

        void Close();

        // // Sets a new wildcard
        bool SetFilter(const char* filter);  

        // Get a line from a file that matches the pattern, if there are no lines then return false
        bool GetNextLine(char* buf, const int bufsize);

        // Injecting a functor which is called each time a line is found which matches the pattern
        void Enumerate(fun f);

    };
}
