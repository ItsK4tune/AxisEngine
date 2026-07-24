#include <core/logic/yaml_parser.h>
#include <core/logic/logger.h>
#include <fstream>
#include <sstream>

YAMLNode* YAMLNode::GetChild(const std::string& k)
{
    for (auto& c : children)
    {
        if (c.key == k)
            return &c;
    }
    return nullptr;
}

const YAMLNode* YAMLNode::GetChild(const std::string& k) const
{
    for (const auto& c : children)
    {
        if (c.key == k)
            return &c;
    }
    return nullptr;
}

std::string YAMLNode::GetChildValue(const std::string& k, const std::string& defaultVal) const
{
    for (auto& c : children)
    {
        if (c.key == k)
            return c.value;
    }
    return defaultVal;
}

void YAMLNode::Merge(YAMLNode& base, const YAMLNode& override)
{
    for (const auto& overChild : override.children)
    {
        bool found = false;
        for (auto& baseChild : base.children)
        {
            bool match = false;
            // For components, we must match by value as well (the component name)
            if (baseChild.key == overChild.key)
            {
                if (baseChild.key == "Component")
                {
                    if (baseChild.value == overChild.value)
                    {
                        match = true;
                    }
                }
                else
                {
                    match = true;
                }
            }

            if (match)
            {
                if (!overChild.value.empty())
                {
                    baseChild.value = overChild.value;
                }

                if (!overChild.children.empty())
                {
                    Merge(baseChild, overChild);
                }

                found = true;
                break;
            }
        }
        if (!found)
        {
            base.children.push_back(overChild);
        }
    }
}

namespace
{
YAMLNode* GetNodeAtPath(std::vector<YAMLNode>& roots, const std::vector<size_t>& path)
{
    if (path.empty() || path[0] >= roots.size())
        return nullptr;
    YAMLNode* current = &roots[path[0]];
    for (size_t i = 1; i < path.size(); ++i)
    {
        if (path[i] >= current->children.size())
            return nullptr;
        current = &current->children[path[i]];
    }
    return current;
}
}  // namespace

std::vector<YAMLNode> YAMLParser::Parse(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        return {};
    return ParseStream(file, filepath);
}

std::vector<YAMLNode> YAMLParser::ParseString(const std::string& content)
{
    std::stringstream ss(content);
    return ParseStream(ss, "<string>");
}

std::vector<YAMLNode> YAMLParser::ParseStream(std::istream& stream, const std::string& sourceName)
{
    std::vector<YAMLNode> roots;
    struct IndentLevel
    {
        int indent;
        std::vector<size_t> path;
    };
    std::vector<IndentLevel> stack;

    std::string line;
    size_t lineNumber = 0;
    while (std::getline(stream, line))
    {
        ++lineNumber;
        if (line.empty())
            continue;

        size_t sourceOffset = 0;
        int indent = 0;
        while (sourceOffset < line.length() && (line[sourceOffset] == ' ' || line[sourceOffset] == '\t'))
        {
            if (line[sourceOffset] == '\t')
            {
                LOGGER_ERROR("YAMLParser") << sourceName << ":" << lineNumber << ":" << (sourceOffset + 1)
                                           << ": tab indentation is not supported; use spaces";
                return {};
            }
            ++sourceOffset;
            ++indent;
        }

        std::string content = line.substr(sourceOffset);
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
            YAMLNode* parent = GetNodeAtPath(roots, stack.back().path);
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
