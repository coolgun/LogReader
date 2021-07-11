#include "Utilities.h"
#include <windows.h>

namespace log_test
{
    void print_last_error(const char* message)
    {
        static constexpr size_t number_buffer = 64;
        OutputDebugStringA("Error message: ");
        OutputDebugStringA(message);
        const auto last_error = GetLastError();
        if (last_error)
        {
            char info[number_buffer] = {};
            _itoa_s(last_error, info, number_buffer, 10);
            OutputDebugStringA(" GetLastError code: ");
            OutputDebugStringA(info);
            SetLastError(0);
        }
        OutputDebugStringA("\n");
    }

    SimpleString::SimpleString(const SimpleString& src)
    {
        Set(src.m_data, src.size);
    }

    SimpleString& SimpleString::operator = (const SimpleString& src)
    {
        Set(src.m_data, src.size);
        return *this;
    }

    SimpleString& SimpleString::operator=(const char* src)
    {
        Set(src, strlen(src));
        return *this;
    }

    SimpleString::~SimpleString()
    {
        Invalidate();
    }

    size_t SimpleString::Size() const
    {
        return size;
    }

    bool SimpleString::IsEmpty() const
    {
        return !size;
    }

    void SimpleString::Invalidate()
    {
        if (m_data)
            HeapFree(GetProcessHeap(), 0, m_data);
        size = 0;
        alloc_size = 0;
    }

    bool SimpleString::PushBack(char ch)
    {
        if (size == alloc_size)
        {
            ReallocBuffer(alloc_size ? 2* alloc_size : default_buffer_size);
            if (!m_data)
                return false;
        }
        m_data[size++] = ch;
        return true;
    }

    const char* SimpleString::Data() const
    {
        return m_data;
    }

    void SimpleString::Reset()
    {
        if (m_data)
            ZeroMemory(m_data, alloc_size);
        size = 0;
    }
   
    void SimpleString::Set(const char* src, size_t new_size)
    {
        if (!src)
        {
            Reset();
            return;
        }
        if (alloc_size < (new_size+1))
        {
            ReallocBuffer(new_size+1);
        }
        size = new_size;
        CopyMemory(m_data, src, new_size);
        m_data[new_size] = 0;
    }

    void SimpleString::ReallocBuffer(size_t new_alloc_size)
    {
        m_data = static_cast<char*>(
            !alloc_size
            ? HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, new_alloc_size)
            : HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_data, new_alloc_size)
            );
        alloc_size = new_alloc_size;
    }
    
}