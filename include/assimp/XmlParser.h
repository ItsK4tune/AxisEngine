

#ifndef INCLUDED_AI_IRRXML_WRAPPER
#define INCLUDED_AI_IRRXML_WRAPPER

#include <assimp/ai_assert.h>
#include <assimp/StringUtils.h>
#include <assimp/DefaultLogger.hpp>

#include "BaseImporter.h"
#include "IOStream.hpp"

#include <pugixml.hpp>
#include <istream>
#include <utility>
#include <vector>

namespace Assimp {


struct find_node_by_name_predicate {
    
    find_node_by_name_predicate() = default;


    std::string mName; 
    find_node_by_name_predicate(const std::string &name) :
            mName(name) {
        
    }

    bool operator()(pugi::xml_node node) const {
        return node.name() == mName;
    }
};



template <class TNodeType>
struct NodeConverter {
public:
    static int to_int(TNodeType &node, const char *attribName) {
        ai_assert(nullptr != attribName);
        return node.attribute(attribName).to_int();
    }
};

using XmlNode = pugi::xml_node;
using XmlAttribute = pugi::xml_attribute;
















template <class TNodeType>
class TXmlParser {
public:
    
    TXmlParser();

    
    ~TXmlParser();

    
    void clear();

    
    
    
    TNodeType *findNode(const std::string &name);

    
    
    
    bool hasNode(const std::string &name);

    
    
    
    bool parse(IOStream *stream);

    
    
    
    bool parse(std::istream &inStream);

    
    
    bool hasRoot() const;

    
    
    pugi::xml_document *getDocument() const;

    
    
    const TNodeType getRootNode() const;

    
    
    TNodeType getRootNode();

    
    
    
    
    static inline bool hasNode(XmlNode &node, const char *name);

    
    
    
    
    static inline bool hasAttribute(XmlNode &xmlNode, const char *name);

    
    
    
    
    
    static inline bool getUIntAttribute(XmlNode &xmlNode, const char *name, unsigned int &val);

    
    
    
    
    
    static inline bool getIntAttribute(XmlNode &xmlNode, const char *name, int &val);

    
    
    
    
    
    static inline bool getRealAttribute(XmlNode &xmlNode, const char *name, ai_real &val);

    
    
    
    
    
    static inline bool getFloatAttribute(XmlNode &xmlNode, const char *name, float &val);

    
    
    
    
    
    static inline bool getDoubleAttribute(XmlNode &xmlNode, const char *name, double &val);

    
    
    
    
    
    static inline bool getStdStrAttribute(XmlNode &xmlNode, const char *name, std::string &val);

    
    
    
    
    
    static inline bool getBoolAttribute(XmlNode &xmlNode, const char *name, bool &val);

    
    
    
    
    static inline bool getValueAsString(XmlNode &node, std::string &text);

    
    
    
    
    static inline bool getValueAsReal(XmlNode &node, ai_real &v);

    
    
    
    
    static inline bool getValueAsFloat(XmlNode &node, float &v);

    
    
    
    
    static inline bool getValueAsInt(XmlNode &node, int &v);

    
    
    
    
    static inline bool getValueAsBool(XmlNode &node, bool &v);

private:
    pugi::xml_document *mDoc;
    TNodeType mCurrent;
    std::vector<char> mData;
};

template <class TNodeType>
inline TXmlParser<TNodeType>::TXmlParser() :
        mDoc(nullptr),
        mData() {
    
}

template <class TNodeType>
inline TXmlParser<TNodeType>::~TXmlParser() {
    clear();
}

template <class TNodeType>
inline void TXmlParser<TNodeType>::clear() {
    if (mData.empty()) {
        if (mDoc) {
            delete mDoc;
        }
        mDoc = nullptr;
        return;
    }

    mData.clear();
    delete mDoc;
    mDoc = nullptr;
}

template <class TNodeType>
inline TNodeType *TXmlParser<TNodeType>::findNode(const std::string &name) {
    if (name.empty()) {
        return nullptr;
    }

    if (nullptr == mDoc) {
        return nullptr;
    }

    find_node_by_name_predicate predicate(name);
    mCurrent = mDoc->find_node(std::move(predicate));
    if (mCurrent.empty()) {
        return nullptr;
    }

    return &mCurrent;
}

template <class TNodeType>
bool TXmlParser<TNodeType>::hasNode(const std::string &name) {
    return nullptr != findNode(name);
}

template <class TNodeType>
bool TXmlParser<TNodeType>::parse(IOStream *stream) {
    if (hasRoot()) {
        clear();
    }

    if (nullptr == stream) {
        ASSIMP_LOG_DEBUG("Stream is nullptr.");
        return false;
    }

    const size_t len = stream->FileSize();
    mData.resize(len + 1);
    memset(&mData[0], '\0', len + 1);
    stream->Read(&mData[0], 1, len);

    mDoc = new pugi::xml_document();
    
    
    pugi::xml_parse_result parse_result = mDoc->load_buffer(&mData[0], mData.size(), pugi::parse_full);
    if (parse_result.status == pugi::status_ok) {
        return true;
    }

    ASSIMP_LOG_DEBUG("Error while parse xml.", std::string(parse_result.description()), " @ ", parse_result.offset);

    return false;
}

template <class TNodeType>
bool TXmlParser<TNodeType>::parse(std::istream &inStream) {
    if (hasRoot()) {
        clear();
    }
    mDoc = new pugi::xml_document();
    pugi::xml_parse_result parse_result = mDoc->load(inStream);
    if (parse_result.status == pugi::status_ok) {
        return true;
    }

    ASSIMP_LOG_DEBUG("Error while parse xml.", std::string(parse_result.description()), " @ ", parse_result.offset);

    return false;
}

template <class TNodeType>
bool TXmlParser<TNodeType>::hasRoot() const {
    return nullptr != mDoc;
}

template <class TNodeType>
pugi::xml_document *TXmlParser<TNodeType>::getDocument() const {
    return mDoc;
}

template <class TNodeType>
const TNodeType TXmlParser<TNodeType>::getRootNode() const {
    static pugi::xml_node none;
    if (nullptr == mDoc) {
        return none;
    }
    return mDoc->root();
}

template <class TNodeType>
TNodeType TXmlParser<TNodeType>::getRootNode() {
    static pugi::xml_node none;
    if (nullptr == mDoc) {
        return none;
    }

    return mDoc->root();
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::hasNode(XmlNode &node, const char *name) {
    pugi::xml_node child = node.find_child(find_node_by_name_predicate(name));
    return !child.empty();
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::hasAttribute(XmlNode &xmlNode, const char *name) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    return !attr.empty();
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getUIntAttribute(XmlNode &xmlNode, const char *name, unsigned int &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_uint();
    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getIntAttribute(XmlNode &xmlNode, const char *name, int &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_int();
    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getRealAttribute(XmlNode &xmlNode, const char *name, ai_real &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }
#ifdef ASSIMP_DOUBLE_PRECISION
    val = attr.as_double();
#else
    val = attr.as_float();
#endif
    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getFloatAttribute(XmlNode &xmlNode, const char *name, float &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_float();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getDoubleAttribute(XmlNode &xmlNode, const char *name, double &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_double();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getStdStrAttribute(XmlNode &xmlNode, const char *name, std::string &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_string();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getBoolAttribute(XmlNode &xmlNode, const char *name, bool &val) {
    pugi::xml_attribute attr = xmlNode.attribute(name);
    if (attr.empty()) {
        return false;
    }

    val = attr.as_bool();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getValueAsString(XmlNode &node, std::string &text) {
    text = std::string();
    if (node.empty()) {
        return false;
    }

    text = node.text().as_string();
    text = ai_trim(text);

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getValueAsReal(XmlNode& node, ai_real& v) {
    if (node.empty()) {
        return false;
    }

    v = node.text().as_float();

    return true;
}


template <class TNodeType>
inline bool TXmlParser<TNodeType>::getValueAsFloat(XmlNode &node, float &v) {
    if (node.empty()) {
        return false;
    }

    v = node.text().as_float();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getValueAsInt(XmlNode &node, int &v) {
    if (node.empty()) {
        return false;
    }

    v = node.text().as_int();

    return true;
}

template <class TNodeType>
inline bool TXmlParser<TNodeType>::getValueAsBool(XmlNode &node, bool &v) {
    if (node.empty()) {
        return false;
    }

    v = node.text().as_bool();

    return true;
}

using XmlParser = TXmlParser<pugi::xml_node>;


class XmlNodeIterator {
public:
    
    enum IterationMode {
        PreOrderMode, 
        PostOrderMode 
    };
    
    
    
    explicit XmlNodeIterator(XmlNode &parent, IterationMode mode) :
            mParent(parent),
            mNodes(),
            mIndex(0) {
        if (mode == PreOrderMode) {
            collectChildrenPreOrder(parent);
        } else {
            collectChildrenPostOrder(parent);
        }
    }

    
    ~XmlNodeIterator() = default;

    
    
    void collectChildrenPreOrder(XmlNode &node) {
        if (node != mParent && node.type() == pugi::node_element) {
            mNodes.push_back(node);
        }
        for (XmlNode currentNode : node.children()) {
            collectChildrenPreOrder(currentNode);
        }
    }

    
    
    void collectChildrenPostOrder(XmlNode &node) {
        for (XmlNode currentNode = node.first_child(); currentNode; currentNode = currentNode.next_sibling()) {
            collectChildrenPostOrder(currentNode);
        }
        if (node != mParent) {
            mNodes.push_back(node);
        }
    }

    
    
    
    bool getNext(XmlNode &next) {
        if (mIndex == mNodes.size()) {
            return false;
        }

        next = mNodes[mIndex];
        ++mIndex;

        return true;
    }

    
    
    size_t size() const {
        return mNodes.size();
    }

    
    
    bool isEmpty() const {
        return mNodes.empty();
    }

    
    void clear() {
        if (mNodes.empty()) {
            return;
        }

        mNodes.clear();
        mIndex = 0;
    }

private:
    XmlNode &mParent;
    std::vector<XmlNode> mNodes;
    size_t mIndex;
};

} 

#endif 
