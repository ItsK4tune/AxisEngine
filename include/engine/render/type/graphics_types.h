#pragma once

#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class IGraphicsContext;
class IBufferManager;
class ITextureManager;
class IRenderTargetManager;
class Bone;

enum class Primitive
{
    Points,
    Lines,
    LineLoop,
    LineStrip,
    Triangles,
    TriangleStrip,
    TriangleFan
};

enum class DataType
{
    Byte,
    UnsignedByte,
    Short,
    UnsignedShort,
    Int,
    UnsignedInt,
    Float,
    Double
};

enum class BufferType
{
    ArrayBuffer,
    ElementArrayBuffer,
    UniformBuffer,
    TextureBuffer,
    TransformFeedbackBuffer,
    PixelPackBuffer,
    PixelUnpackBuffer,
    DrawIndirectBuffer,
    DispatchIndirectBuffer,
    ShaderStorageBuffer,
    QueryBuffer,
    AtomicCounterBuffer
};

enum class BufferUsage
{
    StreamDraw,
    StreamRead,
    StreamCopy,
    StaticDraw,
    StaticRead,
    StaticCopy,
    DynamicDraw,
    DynamicRead,
    DynamicCopy
};

enum class BufferBit
{
    None = 0,
    Color = 1,
    Depth = 2,
    Stencil = 4
};

inline BufferBit operator|(BufferBit a, BufferBit b)
{
    return static_cast<BufferBit>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(BufferBit a, BufferBit b)
{
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

enum class TextureType
{
    Texture1D,
    Texture2D,
    TextureCubeMap,
    Texture3D,
    Texture2DArray,
    TextureCubeMapArray,
    CubeMapPositiveX,
    CubeMapNegativeX,
    CubeMapPositiveY,
    CubeMapNegativeY,
    CubeMapPositiveZ,
    CubeMapNegativeZ
};

enum class TextureFormat
{
    Red,
    RG,
    RGB,
    RGBA,
    DepthComponent,
    DepthStencil,
    Red_Integer
};

enum class InternalFormat
{
    R8,
    RGB8,
    RGBA8,
    RGBA16F,
    R32UI,
    DepthComponent24,
    Depth24Stencil8
};

enum class TextureWrap
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class TextureFilter
{
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

enum class TextureParameter
{
    MinFilter,
    MagFilter,
    WrapS,
    WrapT,
    WrapR,
    BorderColor,
    TextureMaxAnisotropy,
    SwizzleRGBA
};

enum class TextureSwizzle
{
    Red,
    Green,
    Blue,
    Alpha,
    Zero,
    One
};

enum class TextureUnit
{
    Texture0,
    Texture1,
    Texture2,
    Texture3,
    Texture4,
    Texture5,
    Texture6,
    Texture7,
    Texture8,
    Texture9,
    Texture10,
    Texture11,
    Texture12,
    Texture13,
    Texture14,
    Texture15,
    Texture16,
    Texture17,
    Texture18,
    Texture19,
    Texture20,
    Texture21,
    Texture22,
    Texture23,
    Texture24,
    Texture25,
    Texture26,
    Texture27,
    Texture28,
    Texture29,
    Texture30,
    Texture31
};

enum class CullMode
{
    None,
    Front,
    Back,
    FrontAndBack
};

enum class CompareFunc
{
    Never,
    Less,
    Equal,
    Lequal,
    Greater,
    NotEqual,
    Gequal,
    Always
};

enum class StencilOp
{
    Keep,
    Zero,
    Replace,
    Incr,
    IncrWrap,
    Decr,
    DecrWrap,
    Invert
};

enum class PolygonMode
{
    Point,
    Line,
    Fill
};

enum class PixelStoreParam
{
    UnpackAlignment,
    PackAlignment
};

enum class ServerCapability
{
    Blend,
    CullFace,
    DepthTest,
    StencilTest,
    ScissorTest,
    Multisample
};

enum class BlendFactor
{
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    DstColor,
    OneMinusDstColor,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha
};

enum class FrontFace
{
    CW,
    CCW
};

enum class BlendEquation
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

enum class ShaderType
{
    Vertex,
    Fragment,
    Geometry,
    Compute
};

enum class FramebufferTarget
{
    Framebuffer,
    ReadFramebuffer,
    DrawFramebuffer
};

enum class FramebufferAttachment
{
    None,
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    Depth,
    Stencil,
    DepthStencil
};

enum class MemoryBarrierBit
{
    None = 0,
    ShaderImageAccess = 1,
    VertexAttribArray = 2,
    ElementArray = 4,
    Uniform = 8,
    TextureFetch = 16,
    BufferUpdate = 32,
    FrameBuffer = 64,
    All = 127
};

inline MemoryBarrierBit operator|(MemoryBarrierBit a, MemoryBarrierBit b)
{
    return static_cast<MemoryBarrierBit>(static_cast<int>(a) | static_cast<int>(b));
}

enum class FramebufferStatus
{
    Complete,
    Undefined,
    IncompleteAttachment,
    IncompleteMissingAttachment,
    IncompleteDrawBuffer,
    IncompleteReadBuffer,
    Unsupported,
    IncompleteMultisample,
    IncompleteLayerTargets,
    Unknown
};

enum class QueryType
{
    SamplesPassed,
    AnySamplesPassed,
    AnySamplesPassedConservative,
    TimeElapsed,
    Timestamp,
    PrimitivesGenerated,
    TransformFeedbackPrimitivesWritten
};

struct GpuHandle
{
    uint32_t id = 0;
    GpuHandle() = default;
    GpuHandle(uint32_t handleId) : id(handleId)
    {
    }
    operator uint32_t() const
    {
        return id;
    }
    bool IsValid() const
    {
        return id != 0;
    }
    void Reset()
    {
        id = 0;
    }
    bool operator==(const GpuHandle& other) const
    {
        return id == other.id;
    }
    bool operator!=(const GpuHandle& other) const
    {
        return id != other.id;
    }
};

struct GPUCameraData
{
    float projection[16];
    float view[16];
    float viewPos[4];
    float invProjection[16];
    float invView[16];
    float stableProjection[16];
    float invStableProjection[16];
};

struct GPUGlobalLightData
{
    float lightSpaceMatricesDir[16 * 16];
    float lightSpaceMatricesSpot[16 * 16];
    int numDirLights;
    int nrPointLights;
    int nrSpotLights;
    int u_ReceiveShadow;
    float farPlanePoint;
    float farPlaneSpot;
    float pad0;
    float pad1;
};

struct ShaderPorts
{
    float data[8] = {0.0f};
};

struct GPUGlobalData
{
    float time;
    float deltaTime;
    float resolution[2];
    float pad[12];
};

constexpr int MAX_BONE_INFLUENCE = 4;

struct StaticVertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct SkinnedVertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};
struct Texture
{
    enum class PixelDataOwnership
    {
        Borrowed,
        Stbi,
        Malloc,
        Array
    };

    unsigned int id = 0;
    std::string type;
    std::string path;

    unsigned char* pixelData = nullptr;
    std::shared_ptr<unsigned char> pixelDataOwner;
    int width = 0, height = 0, nrComponents = 0;

    Texture() = default;

    Texture(const Texture& other)
        : id(other.id),
          type(other.type),
          path(other.path),
          pixelDataOwner(other.pixelDataOwner),
          width(other.width),
          height(other.height),
          nrComponents(other.nrComponents)
    {
        pixelData = pixelDataOwner ? pixelDataOwner.get() : other.pixelData;
    }

    Texture& operator=(const Texture& other)
    {
        if (this == &other)
            return *this;

        id = other.id;
        type = other.type;
        path = other.path;
        pixelDataOwner = other.pixelDataOwner;
        pixelData = pixelDataOwner ? pixelDataOwner.get() : other.pixelData;
        width = other.width;
        height = other.height;
        nrComponents = other.nrComponents;
        return *this;
    }

    Texture(Texture&& other) noexcept
        : id(other.id),
          type(std::move(other.type)),
          path(std::move(other.path)),
          pixelDataOwner(std::move(other.pixelDataOwner)),
          width(other.width),
          height(other.height),
          nrComponents(other.nrComponents)
    {
        pixelData = pixelDataOwner ? pixelDataOwner.get() : other.pixelData;
        other.pixelData = nullptr;
        other.width = 0;
        other.height = 0;
        other.nrComponents = 0;
    }

    Texture& operator=(Texture&& other) noexcept
    {
        if (this == &other)
            return *this;

        ReleasePixelData();
        id = other.id;
        type = std::move(other.type);
        path = std::move(other.path);
        pixelDataOwner = std::move(other.pixelDataOwner);
        pixelData = pixelDataOwner ? pixelDataOwner.get() : other.pixelData;
        width = other.width;
        height = other.height;
        nrComponents = other.nrComponents;
        other.pixelData = nullptr;
        other.width = 0;
        other.height = 0;
        other.nrComponents = 0;
        return *this;
    }

    void SetPixelData(unsigned char* data, PixelDataOwnership ownership)
    {
        ReleasePixelData();
        pixelData = data;

        if (!data || ownership == PixelDataOwnership::Borrowed)
            return;

        if (ownership == PixelDataOwnership::Array)
        {
            pixelDataOwner = std::shared_ptr<unsigned char>(data, [](unsigned char* ptr) { delete[] ptr; });
        }
        else
        {
            pixelDataOwner = std::shared_ptr<unsigned char>(data, [](unsigned char* ptr) { std::free(ptr); });
        }
    }

    void SetPixelDataCopy(const unsigned char* data, std::size_t size)
    {
        if (!data || size == 0)
        {
            ReleasePixelData();
            return;
        }

        auto* copy = new unsigned char[size];
        std::memcpy(copy, data, size);
        SetPixelData(copy, PixelDataOwnership::Array);
    }

    void ReleasePixelData()
    {
        pixelDataOwner.reset();
        pixelData = nullptr;
    }

    bool OwnsPixelData() const
    {
        return pixelDataOwner != nullptr;
    }
};

#include <resource/unit/bone_info.h>

struct BoneNodeData
{
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<BoneNodeData> children;

    Bone* cachedBone = nullptr;
};

struct GPUDirLight
{
    glm::vec3 direction;
    float shadowIndex;
    glm::vec3 color;
    float intensity;
    glm::vec3 ambient;
    float pad1;
    glm::vec3 diffuse;
    float pad2;
    glm::vec3 specular;
    float pad3;
};

struct GPUPointLight
{
    glm::vec3 position;
    float shadowIndex;
    glm::vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float radius;
    glm::vec3 ambient;
    float pad1;
    glm::vec3 diffuse;
    float pad2;
    glm::vec3 specular;
    float pad3;
};

struct GPUSpotLight
{
    glm::vec3 position;
    float pad0;
    glm::vec3 direction;
    float shadowIndex;
    glm::vec3 color;
    float intensity;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    float radius;
    float pad3;
    float pad4;
    glm::vec3 ambient;
    float pad5;
    glm::vec3 diffuse;
    float pad6;
    glm::vec3 specular;
    float pad7;
};

class GPUFramebuffer
{
public:
    GPUFramebuffer(IGraphicsContext& context, uint32_t handleId) : m_Context(context)
    {
        m_Handle.id = handleId;
    }

    ~GPUFramebuffer();

    GPUFramebuffer(const GPUFramebuffer&) = delete;
    GPUFramebuffer& operator=(const GPUFramebuffer&) = delete;

    GPUFramebuffer(GPUFramebuffer&& other) noexcept : m_Context(other.m_Context), m_Handle(other.m_Handle)
    {
        other.m_Handle.Reset();
    }
    GPUFramebuffer& operator=(GPUFramebuffer&& other) noexcept;

    uint32_t Get() const
    {
        return m_Handle.id;
    }
    void Release()
    {
        m_Handle.Reset();
    }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

class GPUSSBO
{
public:
    GPUSSBO(IGraphicsContext& context, uint32_t handleId) : m_Context(context)
    {
        m_Handle.id = handleId;
    }

    ~GPUSSBO();

    GPUSSBO(const GPUSSBO&) = delete;
    GPUSSBO& operator=(const GPUSSBO&) = delete;

    GPUSSBO(GPUSSBO&& other) noexcept : m_Context(other.m_Context), m_Handle(other.m_Handle)
    {
        other.m_Handle.Reset();
    }
    GPUSSBO& operator=(GPUSSBO&& other) noexcept;

    uint32_t Get() const
    {
        return m_Handle.id;
    }
    void Release()
    {
        m_Handle.Reset();
    }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

class GPUTexture
{
public:
    GPUTexture(IGraphicsContext& context, uint32_t handleId) : m_Context(context)
    {
        m_Handle.id = handleId;
    }

    ~GPUTexture();

    GPUTexture(const GPUTexture&) = delete;
    GPUTexture& operator=(const GPUTexture&) = delete;

    GPUTexture(GPUTexture&& other) noexcept : m_Context(other.m_Context), m_Handle(other.m_Handle)
    {
        other.m_Handle.Reset();
    }
    GPUTexture& operator=(GPUTexture&& other) noexcept;

    uint32_t Get() const
    {
        return m_Handle.id;
    }
    void Release()
    {
        m_Handle.Reset();
    }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

class GPUUBO
{
public:
    GPUUBO(IGraphicsContext& context, uint32_t handleId) : m_Context(context)
    {
        m_Handle.id = handleId;
    }

    ~GPUUBO();

    GPUUBO(const GPUUBO&) = delete;
    GPUUBO& operator=(const GPUUBO&) = delete;

    GPUUBO(GPUUBO&& other) noexcept : m_Context(other.m_Context), m_Handle(other.m_Handle)
    {
        other.m_Handle.Reset();
    }
    GPUUBO& operator=(GPUUBO&& other) noexcept;

    uint32_t Get() const
    {
        return m_Handle.id;
    }
    void Release()
    {
        m_Handle.Reset();
    }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

constexpr int BLOOM_MIP_COUNT = 6;
