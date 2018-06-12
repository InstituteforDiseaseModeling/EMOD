/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

class Stopwatch
{
public:
    Stopwatch(bool start = false);

    void Start();
    void Stop();
    double ResultNanoseconds();
    void PrintResultSeconds(const char * label);

private:
    uint64_t _start;
    uint64_t _stop;
};

#define REPORT_TIME(enable, label, block) \
{\
    Stopwatch s;\
    if (enable) { s.Start(); }\
    block;\
    if (enable) \
    {\
        s.Stop();\
        s.PrintResultSeconds(label);\
    }\
}

#define BEGIN_REPORT_TIME(enable) \
{\
    Stopwatch s;\
    if (enable) { s.Start(); }

#define END_REPORT_TIME(enable, label) \
    if (enable) \
    {\
        s.Stop();\
        s.PrintResultSeconds(label);\
    }\
}
