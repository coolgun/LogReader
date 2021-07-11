#include "LogReader.h"
#include "SimpleRegexp.h"
#include <process.h>

namespace log_test
{
    class CTextFile final
    {
        static constexpr unsigned long long offset_mask = 0xFFFFFFFFul;
    public:

        CTextFile() = default;

        CTextFile(const char* file_name)
        {
            Open(file_name);
        }

        ~CTextFile()
        {
            Reset();
        }

        // Read the characters and put to buffer the strings up to \\r\\n
        const char* ReadLine()
        {
            if (Eof()) return {};
            bool bad_alloc_flag = false;
            current_line.Reset();

            for (size_t i{}; IsOpen(); ++i)
            {
                const auto ch = ReadByte();
                if (ch == '\r')
                {
                    if (Eof() || (ReadByte() == '\n'))
                    {
                        if (!current_line.PushBack(0))
                        {
                            bad_alloc_flag = true;
                            break;
                        }
                        return current_line.Data();
                    };
                    break;
                }
                if (!current_line.PushBack(ch))
                {
                    bad_alloc_flag = true;
                    break;
                }
            }

            print_last_error(bad_alloc_flag ? "Bad alloc" : "Bad file");
            Reset();
            return {};
        }

        bool Eof() const
        {
            return current_pos >= file_size;
        }

        bool IsOpen() const
        {
            return is_open;
        }

        // Open the file and create a memory mapped object
        void Open(const char* file_name)
        {
            Reset();

            hFile = CreateFileA(
                file_name,
                GENERIC_READ,
                FILE_SHARE_READ,
                0,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                0);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                print_last_error("CreateFileA");
                return;
            }

            LARGE_INTEGER filesize{};

            if (!GetFileSizeEx(hFile, &filesize))
            {
                print_last_error("GetFileSizeEx");
                Reset();
                return;
            }

            file_size = static_cast<unsigned long long>(filesize.QuadPart);

            hMapFile = CreateFileMapping(hFile, nullptr, PAGE_READONLY, filesize.HighPart, filesize.LowPart, nullptr);
            if (!hMapFile)
            {
                print_last_error("CreateFileMapping");
                Reset();
                return;
            }
            is_open = true;
            NextMapView();
        }

        // release all resources
        void Reset()
        {
            is_open = false;
            file_size = 0;
            current_pos = 0;
            UnMapView();
            if (hMapFile)
            {
                CloseHandle(hMapFile);
                hMapFile = NULL;
            }

            if (hFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;
            }
        }

    private:
        // Reads the next byte from the MapView, if the MapView is over, shifts further by chunk_size
        char ReadByte()
        {
            if (!current_chunk_size)
            {
                NextMapView();
            }
            --current_chunk_size;
            ++current_pos;
            return *pos_map_view++;
        }
        
        // Shifts further by chunk_size
        void NextMapView()
        {
            UnMapView();
            const auto high = static_cast<DWORD>((offset >> 32) & offset_mask);
            const auto low = static_cast<DWORD>(offset & offset_mask);
            current_chunk_size = (offset + chunk_size) > file_size ? static_cast<int>(file_size - offset) : chunk_size;
            offset += current_chunk_size;
            map_view = pos_map_view = static_cast<char*>(MapViewOfFile(hMapFile, FILE_MAP_READ, high, low, current_chunk_size));
            if (!map_view)
            {
                print_last_error("MapViewOfFile");
                Reset();
            }
        }

        void UnMapView()
        {
            if (map_view)
            {
                UnmapViewOfFile(map_view);
                map_view = {};
            }
        }
      

    private:

        // Set chunk size of the MapView according to granularity
        const DWORD chunk_size = []
        {
            SYSTEM_INFO sysinfo = { 0 };
            ::GetSystemInfo(&sysinfo);
            return sysinfo.dwAllocationGranularity;
        }();

        // Size of the current chunk , for the last it may be less than chunk_size
        DWORD  current_chunk_size{};
        HANDLE hFile = INVALID_HANDLE_VALUE;
        HANDLE hMapFile{};
        unsigned long long file_size{};
        // the sequence number of the current byte in the file
        unsigned long long current_pos{};
        // Pointer to the beginning of mapped memory
        const char* map_view{};
        const char* pos_map_view{};
        unsigned long long offset{};
        bool is_open{};
        // Current line buffer
        SimpleString current_line;
    };

    CLogReader::CLogReader(const char* filter) :
        text_file(new CTextFile),
        reg_exp(new CSimpleRegexp(filter))
    {}

    CLogReader::~CLogReader()
    {
        delete text_file;
        delete reg_exp;
    }

    // открытие файла, false - ошибка
    bool CLogReader::Open(const char* file_name)
    {
        text_file->Open(file_name);
        return text_file->IsOpen();
    }

    void CLogReader::Close() // закрытие файла
    {
        text_file->Reset();
    }

    // установка фильтра строк, false - ошибка
    bool CLogReader::SetFilter(const char* filter)
    {
        return reg_exp->SetFilter(filter);
    }

    bool CLogReader::GetNextLine(char* buf, const int bufsize)
    {
        if (bufsize <= 0) return false;
        if (!text_file->IsOpen()) return false;
        if (!reg_exp->IsOk()) return false;
        
        for (;;)
        {
            const auto* line = text_file->ReadLine();
            if (!line || !text_file->IsOpen()) return false;
            if (reg_exp->Match(line))
            {
                CopyMemory(buf, line, (min(static_cast<size_t>(bufsize), strlen(line) + 1)));
                return true;
            }
        }
    }

   void CLogReader::Enumerate(Fun f)
    {
        if (!text_file->IsOpen()) return;
        if (!reg_exp->IsOk()) return;

        for (;;)
        {
            const auto* line = text_file->ReadLine();
            if (!line || !text_file->IsOpen()) return;
            if (reg_exp->Match(line))
            {
                f(line, strlen(line));
            }
        }
    }
}


namespace log_test
{
    class AsyncEnumerateHelper
    {
    public:
        AsyncEnumerateHelper(CLogReader *p_log_reader, Fun f)
            : log_reader(p_log_reader)
            , fun(f)
            , stop(false)
        {
            
        }
        void process()
        {
         
            InitializeConditionVariable(&buffer_not_empty);
            InitializeConditionVariable(&buffer_not_full);

            InitializeCriticalSection(&buffer_lock);

            HANDLE hReadLine = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, ReadLineThreadProc, this, 0, 0));
            HANDLE hMatch = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, MatchThreadProc, this, 0, 0));

            WaitForSingleObject(hReadLine, INFINITE);

            EnterCriticalSection(&buffer_lock);
            stop = true;
            LeaveCriticalSection(&buffer_lock);
            WaitForSingleObject(hMatch, INFINITE);
        }
    private:
        unsigned ReadLines()
        {
            if (!log_reader->text_file->IsOpen()) return 0;
            if (!log_reader->reg_exp->IsOk()) return 0;

            for (;;)
            {
                const auto* line = log_reader->text_file->ReadLine();
                if (!line || !log_reader->text_file->IsOpen()) return 0;

                EnterCriticalSection(&buffer_lock);

                while (queue_size == BUFFER_SIZE && !stop)
                {
                    SleepConditionVariableCS(&buffer_not_full, &buffer_lock, INFINITE);
                }

                if (stop)
                {
                    LeaveCriticalSection(&buffer_lock);
                    break;
                }

                circular_buffer[(queue_start_offset + queue_size) % BUFFER_SIZE] = line;
                ++queue_size;
                LeaveCriticalSection(&buffer_lock);
                WakeConditionVariable(&buffer_not_empty);
            }
            return 0;
        }

        static unsigned WINAPI ReadLineThreadProc(void* data)
        {
             auto* this_ = reinterpret_cast<AsyncEnumerateHelper*>(data);
             return this_->ReadLines();
        }

        unsigned MatchLines()
        {
            SimpleString current_line;
            while (true)
            {
                EnterCriticalSection(&buffer_lock);

                while (queue_size == 0 && !stop)
                {
                    SleepConditionVariableCS(&buffer_not_empty, &buffer_lock, INFINITE);
                }

                if (stop && queue_size == 0)
                {
                    LeaveCriticalSection(&buffer_lock);
                    break;
                }

                current_line = circular_buffer[queue_start_offset];

                --queue_size;
                ++queue_start_offset;

                if (queue_start_offset == BUFFER_SIZE)
                {
                    queue_start_offset = 0;
                }

                LeaveCriticalSection(&buffer_lock);

                WakeConditionVariable(&buffer_not_full);
                const auto* line = current_line.Data();
                if (log_reader->reg_exp->Match(line))
                {
                    fun(line, strlen(line));
                }
            }

            return 0;
        };

        static unsigned WINAPI MatchThreadProc(void* data)
        {
            auto* this_ = reinterpret_cast<AsyncEnumerateHelper*>(data);
            return this_->MatchLines();
        }

        
    private:   
        CLogReader* log_reader;
        Fun fun;
        static constexpr size_t BUFFER_SIZE = 100;
        
        SimpleString circular_buffer[BUFFER_SIZE];

        size_t queue_size{};
        size_t queue_start_offset{};

        CONDITION_VARIABLE buffer_not_empty;
        CONDITION_VARIABLE buffer_not_full;
        CRITICAL_SECTION   buffer_lock;
        bool stop;
    };

    void CLogReader::AsyncEnumerate(Fun f)
    {
        AsyncEnumerateHelper helper(this, f);
        helper.process();
    }
}