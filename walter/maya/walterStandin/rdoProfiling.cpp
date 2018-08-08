// Copyright 2017 Rodeo FX.  All rights reserved.

#include "rdoProfiling.h"

#include <sys/syscall.h>
#include <unistd.h>

std::map<size_t, std::stack<RdoProfiling*>> RdoProfiling::s_stacks;

RdoProfiling::RdoProfiling(
    const char* i_function,
    const char* i_file,
    int i_line,
    const char* i_message) :
        m_function(i_function),
        m_file(i_file),
        m_line(i_line),
        m_message(i_message),
        m_start(std::chrono::high_resolution_clock::now()),
        m_others(0),
        mThreadID(syscall(SYS_gettid))
{
    std::stack<RdoProfiling*>& stack = s_stacks[mThreadID];

    stack.push(this);

    std::string indent;
    for (unsigned int i = 0; i < stack.size(); i++)
    {
        indent += " ";
    }

    printf(
        "%s{ <%zu> %s:%i %s: %s\n",
        indent.c_str(),
        mThreadID,
        m_file.c_str(),
        m_line,
        m_function.c_str(),
        m_message.c_str());
}

RdoProfiling::~RdoProfiling()
{
    std::stack<RdoProfiling*>& stack = s_stacks[mThreadID];

    std::chrono::high_resolution_clock::time_point finish =
        std::chrono::high_resolution_clock::now();

    auto delta = finish - m_start;
    int duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();

    std::string indent;
    for (unsigned int i = 0; i < stack.size(); i++)
    {
        indent += " ";
    }

    printf(
        "%s} <%zu> %ims %s: %s\n",
        indent.c_str(),
        mThreadID,
        duration - m_others,
        m_function.c_str(),
        m_message.c_str());

    stack.pop();

    if (stack.size())
    {
        stack.top()->m_others += duration;
    }
}
