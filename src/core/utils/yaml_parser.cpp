#include <engine/core/utils/yaml_parser.h>
#include <fstream>
#include <sstream>

YAMLNode *YAMLNode::GetChild(const std::string &k)
{
    for (auto &c : children)
    {
        if (c.key == k)
            return &c;
    }
    return nullptr;
}

std::string YAMLNode::GetChildValue(const std::string &k, const std::string &defaultVal) const
{
    for (auto &c : children)
    {
        if (c.key == k)
            return c.value;
    }
    return defaultVal;
}

namespace
{
    YAMLNode *GetNodeAtPath(std::vector<YAMLNode> &roots, const std::vector<size_t> &path)
    {
        if (path.empty())
            return nullptr;
        YAMLNode *current = &roots[path[0]];
        for (size_t i = 1; i < path.size(); ++i)
        {
            current = &current->children[path[i]];
        }
        return current;
    }
}

std::vector<YAMLNode> YAMLParser::Parse(const std::string &filepath)
{
    std::vector<YAMLNode> roots;
    std::ifstream file(filepath);
    if (!file.is_open())
        return roots;

    struct IndentLevel
    {
        int indent;
        std::vector<size_t> path;
    };
    std::vector<IndentLevel> stack;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        int indent = 0;
        while (indent < (int)line.length() && (line[indent] == ' ' || line[indent] == '\t'))
        {
            indent++;
            if (line[indent - 1] == '\t')
            {
                indent++;
            }
        }

        std::string content = line.substr(indent);
        if (content.empty() || content[0] == '#')
            continue;

        auto colonPos = content.find(':');

        std::string key = content;
        std::string value = "";

        if (colonPos != std::string::npos)
        {
            key = content.substr(0, colonPos);
            if (colonPos + 1 < content.length())
            {
                value = content.substr(colonPos + 1);
                size_t start = value.find_first_not_of(" \t\r\n");
                if (start != std::string::npos)
                {
                    value = value.substr(start);
                    size_t end = value.find_last_not_of(" \t\r\n");
                    if (end != std::string::npos)
                        value = value.substr(0, end + 1);
                }
                else
                {
                    value = "";
                }
            }
        }
        else if (content.length() > 2 && content[0] == '-' && content[1] == ' ')
        {
            key = "-";
            value = content.substr(2);
        }

        YAMLNode newNode{key, value, {}};

        while (!stack.empty() && stack.back().indent >= indent)
        {
            stack.pop_back();
        }

        if (stack.empty())
        {
            roots.push_back(newNode);
            stack.push_back({indent, {roots.size() - 1}});
        }
        else
        {
            YAMLNode *parent = GetNodeAtPath(roots, stack.back().path);
            if (parent)
            {
                parent->children.push_back(newNode);

                std::vector<size_t> currentPath = stack.back().path;
                currentPath.push_back(parent->children.size() - 1);
                stack.push_back({indent, currentPath});
            }
        }
    }
    return roots;
}
