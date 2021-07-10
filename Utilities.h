#pragma once

namespace log_test
{
    // Prints an error message and GetLastError code
    void print_last_error(const char* message);

    /*********************************************************************************************
    /*
    /* A naive implementation of a string class that can add character by character and 
    /* automatically expand the buffer
    /*
    /*********************************************************************************************/
    class SimpleString final
    {
    public:
        SimpleString() = default;
        ~SimpleString();
        bool PushBack(char ch);
        size_t Size() const;
        bool IsEmpty() const;
        const char* Data() const;
        void Reset();
        void Invalidate();
    private:
        void ReallocBuffer();
        size_t alloc_size{};
        size_t size{};
        char* m_data{};
        static constexpr unsigned long long default_buffer_size = 256;
    };
}