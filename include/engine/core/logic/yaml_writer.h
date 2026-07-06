#pragma once

#include <core/logic/yaml_parser.h>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

class YAMLWriter
{
public:
    static constexpr int IndentWidth = 2;

    static void Write(std::ostream& stream,
                      const std::vector<YAMLNode>& roots,
                      int baseIndent = 0);
    static bool WriteFile(const std::string& filepath,
                          const std::vector<YAMLNode>& roots,
                          int baseIndent = 0);
    static std::string WriteString(const std::vector<YAMLNode>& roots,
                                   int baseIndent = 0);
    static void WriteSection(std::ostream& stream,
                             const std::string& sectionKey,
                             const std::vector<YAMLNode>& roots);

private:
    static void WriteNode(std::ostream& stream,
                          const YAMLNode& node,
                          int indent);
};

class BinaryYAMLWriter
{
public:
    static constexpr uint32_t MAGIC   = 0x59414D4C;
    static constexpr uint8_t  VERSION = 1;

    static void Write(std::ostream& stream,
                      const std::vector<YAMLNode>& roots);
    static bool WriteFile(const std::string& filepath,
                          const std::vector<YAMLNode>& roots);
    static std::vector<YAMLNode> Read(std::istream& stream);
    static std::vector<YAMLNode> ReadFile(const std::string& filepath);

private:
    static void WriteNode(std::ostream& stream, const YAMLNode& node);
    static bool ReadNode(std::istream& stream, YAMLNode& node);

    static void     WriteU32(std::ostream& stream, uint32_t v);
    static bool     ReadU32(std::istream& stream, uint32_t& v);
    static void     WriteStr(std::ostream& stream, const std::string& s);
    static bool     ReadStr(std::istream& stream, std::string& s);

    static constexpr uint32_t MAX_STRING_BYTES = 1024u * 1024u;
    static constexpr uint32_t MAX_NODES        = 1024u * 1024u;
};
