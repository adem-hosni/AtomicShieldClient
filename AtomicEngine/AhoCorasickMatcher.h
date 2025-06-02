#pragma once
#include "StdInc.h"

class CAhoCorasickMatcher
{
public:
    CAhoCorasickMatcher();
    ~CAhoCorasickMatcher();

    void                                        Build(const std::vector<std::string>& patterns);
    std::vector<std::pair<size_t, std::string>> Search(const char* data, size_t length);

private:
    struct Node
    {
        std::map<char, Node*>    children;
        Node*                    fail = nullptr;
        std::vector<std::string> outputs;
    };

    Node* m_pRoot;

    void Insert(const std::string& pattern);
    void FreeNode(Node* node);
};
