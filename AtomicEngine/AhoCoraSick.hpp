#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>

class AhoCorasick
{
private:
    struct Node
    {
        std::unordered_map<char, int> next;
        int                           fail = 0;
        std::vector<int>              output;
    };

    std::vector<Node> trie;
    int               patternCount = 0;

public:
    AhoCorasick()
    {
        trie.emplace_back();            // root node
    }

    void insert(const std::string& word, int id)
    {
        int current = 0;
        for (char ch : word)
        {
            if (!trie[current].next.count(ch))
            {
                trie[current].next[ch] = trie.size();
                trie.emplace_back();
            }
            current = trie[current].next[ch];
        }
        trie[current].output.push_back(id);
    }

    void build()
    {
        std::queue<int> q;
        for (auto& [ch, node] : trie[0].next)
        {
            trie[node].fail = 0;
            q.push(node);
        }

        while (!q.empty())
        {
            int current = q.front();
            q.pop();
            for (auto& [ch, next] : trie[current].next)
            {
                int fail = trie[current].fail;
                while (fail && !trie[fail].next.count(ch))
                    fail = trie[fail].fail;
                if (trie[fail].next.count(ch))
                    fail = trie[fail].next[ch];
                trie[next].fail = fail;
                trie[next].output.insert(trie[next].output.end(), trie[fail].output.begin(), trie[fail].output.end());
                q.push(next);
            }
        }
    }

    // Returns matches as pairs: (position in text, pattern ID)
    std::vector<std::pair<int, int>> search(const std::string& text)
    {
        std::vector<std::pair<int, int>> results;
        int                              current = 0;

        for (int i = 0; i < text.size(); ++i)
        {
            char ch = text[i];
            while (current && !trie[current].next.count(ch))
                current = trie[current].fail;
            if (trie[current].next.count(ch))
                current = trie[current].next[ch];
            for (int id : trie[current].output)
                results.emplace_back(i, id);
        }

        return results;
    }
};
