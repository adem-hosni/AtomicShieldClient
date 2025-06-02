#include "StdInc.h"

CAhoCorasickMatcher::CAhoCorasickMatcher()
{
    m_pRoot = new Node();
}

CAhoCorasickMatcher::~CAhoCorasickMatcher()
{
    FreeNode(m_pRoot);
}

void CAhoCorasickMatcher::Build(const std::vector<std::string>& patterns)
{
    for (const auto& pattern : patterns)
        Insert(pattern);

    std::queue<Node*> q;
    for (auto& [_, child] : m_pRoot->children)
    {
        child->fail = m_pRoot;
        q.push(child);
    }

    while (!q.empty())
    {
        Node* current = q.front();
        q.pop();

        for (auto& [ch, child] : current->children)
        {
            Node* fail = current->fail;
            while (fail && !fail->children.count(ch))
                fail = fail->fail;

            child->fail = fail ? fail->children[ch] : m_pRoot;
            child->outputs.insert(child->outputs.end(), child->fail->outputs.begin(), child->fail->outputs.end());

            q.push(child);
        }
    }
}

void CAhoCorasickMatcher::Insert(const std::string& pattern)
{
    Node* node = m_pRoot;
    for (char ch : pattern)
    {
        if (!node->children.count(ch))
            node->children[ch] = new Node();
        node = node->children[ch];
    }
    node->outputs.push_back(pattern);
}

void CAhoCorasickMatcher::FreeNode(Node* node)
{
    for (auto& [_, child] : node->children)
        FreeNode(child);
    delete node;
}

std::vector<std::pair<size_t, std::string>> CAhoCorasickMatcher::Search(const char* data, size_t length)
{
    std::vector<std::pair<size_t, std::string>> matches;
    Node*                                       node = m_pRoot;

    for (size_t i = 0; i < length; ++i)
    {
        char ch = data[i];
        while (node && !node->children.count(ch))
            node = node->fail;

        if (!node)
            node = m_pRoot;
        else
            node = node->children[ch];

        for (const auto& out : node->outputs)
            matches.emplace_back(i - out.size() + 1, out);
    }

    return matches;
}
