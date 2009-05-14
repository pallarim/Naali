// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "Profiler.h"

namespace Foundation
{
    bool ProfilerBlock::supported_ = false;

#ifdef _WINDOWS
    LARGE_INTEGER ProfilerBlock::frequency_;
    LARGE_INTEGER ProfilerBlock::api_overhead_;
#endif

//    Profiler *ProfilerSection::profiler_ = new Profiler;

    bool ProfilerBlock::QueryCapability()
    {
#ifdef _WINDOWS
        BOOL result = QueryPerformanceFrequency(&frequency_);
        supported_ = (result != 0);

        ProfilerBlock cnt;
        cnt.Start();
        cnt.Stop();
        api_overhead_.QuadPart = cnt.end_time_.QuadPart - cnt.start_time_.QuadPart;
#endif
        return supported_;
    }

    void Profiler::StartBlock(const std::string &name)
    {
        ProfilerNodeTree *node = current_node_->GetChild(name);
        if (!node)
        {
            node = new ProfilerNode(name, current_node_);
            current_node_->AddChild(node);
        }

        current_node_ = node;
        checked_static_cast<ProfilerNode*>(current_node_)->block_.Start();
    }

    void Profiler::EndBlock(const std::string &name)
    {
        assert (current_node_->Name() == name && "New profiling block started before old one ended!");

        ProfilerNode* node = checked_static_cast<ProfilerNode*>(current_node_);
        node->block_.Stop();
        node->num_called_total_++;

        double elapsed = node->block_.ElapsedTimeSeconds();

        node->elapsed_current_ += elapsed;
        node->total_ += elapsed;

        current_node_ = node->Parent();
    }
}
