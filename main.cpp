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
    char buf[256] = {};
    if (!reader.Open(argv[1])) return -1;
    
#if 1 
    while (reader.GetNextLine(buf, 256))
    {
        printf("%s\n", buf);
    }
#else
    reader.Enumerate([](const char* buf, size_t bufsize)
        {
            printf("%s\n", buf);
        }
    );
#endif
    return 0;
}
