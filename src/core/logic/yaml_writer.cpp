#include <core/logic/yaml_writer.h>
#include <fstream>
#include <sstream>

// ===========================================================================
// YAMLWriter
// ===========================================================================

void YAMLWriter::WriteNode(std::ostream& stream, const YAMLNode& node, int indent)
{
    // Emit leading spaces
    for (int i = 0; i < indent; ++i)
        stream << ' ';

    if (node.key == "-")
    {
        // List item: "- value\n"
        stream << "- " << node.value << '\n';
    }
    else if (node.value.empty())
    {
        // Key-only (section header or node that holds children only)
        stream << node.key << ":\n";
    }
    else
    {
        // "key: value\n"
        stream << node.key << ": " << node.value << '\n';
    }

    for (const auto& child : node.children)
        WriteNode(stream, child, indent + IndentWidth);
}

void YAMLWriter::Write(std::ostream& stream,
                       const std::vector<YAMLNode>& roots,
                       int baseIndent)
{
    for (const auto& root : roots)
        WriteNode(stream, root, baseIndent);
}

bool YAMLWriter::WriteFile(const std::string& filepath,
                           const std::vector<YAMLNode>& roots,
                           int baseIndent)
{
    std::ofstream f(filepath);
    if (!f.is_open())
        return false;
    Write(f, roots, baseIndent);
    return f.good();
}

std::string YAMLWriter::WriteString(const std::vector<YAMLNode>& roots,
                                    int baseIndent)
{
    std::ostringstream ss;
    Write(ss, roots, baseIndent);
    return ss.str();
}

void YAMLWriter::WriteSection(std::ostream& stream,
                              const std::string& sectionKey,
                              const std::vector<YAMLNode>& roots)
{
    stream << sectionKey << ":\n";
    Write(stream, roots, IndentWidth);
}

// ===========================================================================
// BinaryYAMLWriter — helpers
// ===========================================================================

void BinaryYAMLWriter::WriteU32(std::ostream& stream, uint32_t v)
{
    stream.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

bool BinaryYAMLWriter::ReadU32(std::istream& stream, uint32_t& v)
{
    stream.read(reinterpret_cast<char*>(&v), sizeof(v));
    return stream.good();
}

void BinaryYAMLWriter::WriteStr(std::ostream& stream, const std::string& s)
{
    const uint32_t len = static_cast<uint32_t>(
        (s.size() < MAX_STRING_BYTES) ? s.size() : MAX_STRING_BYTES);
    WriteU32(stream, len);
    if (len > 0)
        stream.write(s.data(), len);
}

bool BinaryYAMLWriter::ReadStr(std::istream& stream, std::string& s)
{
    uint32_t len = 0;
    if (!ReadU32(stream, len))
        return false;
    if (len > MAX_STRING_BYTES)
    {
        stream.setstate(std::ios::failbit);
        return false;
    }
    s.resize(len);
    if (len > 0)
        stream.read(s.data(), len);
    return stream.good();
}

// ===========================================================================
// BinaryYAMLWriter — node I/O
// ===========================================================================

void BinaryYAMLWriter::WriteNode(std::ostream& stream, const YAMLNode& node)
{
    WriteStr(stream, node.key);
    WriteStr(stream, node.value);

    const uint32_t childCount = static_cast<uint32_t>(node.children.size());
    WriteU32(stream, childCount);

    for (const auto& child : node.children)
        WriteNode(stream, child);
}

bool BinaryYAMLWriter::ReadNode(std::istream& stream, YAMLNode& node)
{
    if (!ReadStr(stream, node.key))
        return false;
    if (!ReadStr(stream, node.value))
        return false;

    uint32_t childCount = 0;
    if (!ReadU32(stream, childCount))
        return false;
    if (childCount > MAX_NODES)
    {
        stream.setstate(std::ios::failbit);
        return false;
    }

    node.children.resize(childCount);
    for (auto& child : node.children)
    {
        if (!ReadNode(stream, child))
            return false;
    }
    return true;
}

// ===========================================================================
// BinaryYAMLWriter — public API
// ===========================================================================

void BinaryYAMLWriter::Write(std::ostream& stream,
                             const std::vector<YAMLNode>& roots)
{
    WriteU32(stream, MAGIC);
    const uint8_t ver = VERSION;
    stream.write(reinterpret_cast<const char*>(&ver), 1);

    const uint32_t rootCount = static_cast<uint32_t>(roots.size());
    WriteU32(stream, rootCount);

    for (const auto& root : roots)
        WriteNode(stream, root);
}

bool BinaryYAMLWriter::WriteFile(const std::string& filepath,
                                 const std::vector<YAMLNode>& roots)
{
    std::ofstream f(filepath, std::ios::binary);
    if (!f.is_open())
        return false;
    Write(f, roots);
    return f.good();
}

std::vector<YAMLNode> BinaryYAMLWriter::Read(std::istream& stream)
{
    uint32_t magic = 0;
    if (!ReadU32(stream, magic) || magic != MAGIC)
        return {};

    uint8_t ver = 0;
    stream.read(reinterpret_cast<char*>(&ver), 1);
    if (!stream.good() || ver != VERSION)
        return {};

    uint32_t rootCount = 0;
    if (!ReadU32(stream, rootCount) || rootCount > MAX_NODES)
        return {};

    std::vector<YAMLNode> roots(rootCount);
    for (auto& root : roots)
    {
        if (!ReadNode(stream, root))
            return {};
    }
    return roots;
}

std::vector<YAMLNode> BinaryYAMLWriter::ReadFile(const std::string& filepath)
{
    std::ifstream f(filepath, std::ios::binary);
    if (!f.is_open())
        return {};
    return Read(f);
}
