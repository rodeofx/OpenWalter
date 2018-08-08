// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef _RDOPROFILING_H_
#define _RDOPROFILING_H_

#include <chrono>
#include <map>
#include <stack>
#include <string>

// Profiling output. Just add RDOPROFILE to the begin of a function to output
// the execution time.
#ifdef RDO_ENABLE_PROFILE
#define RDOPROFILE(message) \
    RdoProfiling __prf(__FUNCTION__, __FILE__, __LINE__, message)
#else
#define RDOPROFILE(message)
#endif

// TODO: It only works in a single thread.
class RdoProfiling
{
public:
    RdoProfiling(
        const char* i_function,
        const char* i_file,
        int i_line,
        const char* i_message);
    ~RdoProfiling();

private:
    static std::map<size_t, std::stack<RdoProfiling*>> s_stacks;

    std::chrono::high_resolution_clock::time_point m_start;
    unsigned int m_others;
    std::string m_function;
    std::string m_file;
    std::string m_message;
    int m_line;

    // Current thread id.
    size_t mThreadID;
};

#endif
