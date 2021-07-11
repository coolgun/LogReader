#include "LogReader.h"

#include <shellapi.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("there should be 2 parameters\n");
        return -1;
    }

    log_test::CLogReader reader;
    if (!reader.SetFilter(argv[2])) return -1;
    if (!reader.Open(argv[1])) return -1;

#if 0
    static constexpr size_t buffer_size = 256;
    char buf[buffer_size] = {};
    while (reader.GetNextLine(buf, buffer_size))
    {
        printf("%s\n", buf);
    }
#elif 0  
    reader.Enumerate([](const char* buf, size_t)
        {
            printf("%s\n", buf);
        }
    );
#else  
    reader.AsyncEnumerate([](const char* buf, size_t)
        {
            printf("%s\n", buf);
        }
    );
#endif
    return 0;
}
