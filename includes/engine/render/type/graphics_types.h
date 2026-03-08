#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>


// Forward declarations for resource wrappers
class IGraphicsContext;
class IBufferManager;
class ITextureManager;
class IRenderTargetManager;
class Bone;


// --- Common Types ---

enum class Primitive {
    Points,
    Lines,
    LineLoop,
    LineStrip,
    Triangles,
    TriangleStrip,
    TriangleFan
};

enum class DataType {
    Byte,
    UnsignedByte,
    Short,
    UnsignedShort,
    Int,
    UnsignedInt,
    Float,
    Double
};

// --- Buffer Types ---

enum class BufferType {
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

enum class BufferUsage {
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

enum class BufferBit {
    None = 0,
    Color = 1,
    Depth = 2,
    Stencil = 4
};

inline BufferBit operator|(BufferBit a, BufferBit b) {
    return static_cast<BufferBit>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(BufferBit a, BufferBit b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

// --- Texture Types ---

enum class TextureType {
    Texture2D,
    TextureCubeMap,
    Texture3D,
    CubeMapPositiveX,
    CubeMapNegativeX,
    CubeMapPositiveY,
    CubeMapNegativeY,
    CubeMapPositiveZ,
    CubeMapNegativeZ
};

enum class TextureFormat {
    Red,
    RG,
    RGB,
    RGBA,
    DepthComponent,
    DepthStencil
};

enum class InternalFormat {
    R8,
    RGB8,
    RGBA8,
    RGBA16F,
    DepthComponent24,
    Depth24Stencil8
};

enum class TextureWrap {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class TextureFilter {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

enum class TextureParameter {
    MinFilter,
    MagFilter,
    WrapS,
    WrapT,
    WrapR,
    BorderColor
};

enum class TextureUnit {
    Texture0,
    Texture1,
    Texture2,
    Texture3,
    Texture4,
    Texture5,
    Texture6,
    Texture7
};

// --- Render State Types ---

enum class CullMode {
    None,
    Front,
    Back,
    FrontAndBack
};

enum class CompareFunc {
    Never,
    Less,
    Equal,
    Lequal,
    Greater,
    NotEqual,
    Gequal,
    Always
};

enum class StencilOp {
    Keep,
    Zero,
    Replace,
    Incr,
    IncrWrap,
    Decr,
    DecrWrap,
    Invert
};

enum class PolygonMode {
    Point,
    Line,
    Fill
};

enum class PixelStoreParam {
    UnpackAlignment,
    PackAlignment
};

enum class ServerCapability {
    Blend,
    CullFace,
    DepthTest,
    StencilTest,
    ScissorTest,
    Multisample
};

enum class BlendFactor {
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

enum class FrontFace {
    CW,
    CCW
};

enum class BlendEquation {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

// --- Framebuffer & Shader Types ---

enum class ShaderType {
    Vertex,
    Fragment,
    Geometry,
    Compute
};

enum class FramebufferTarget {
    Framebuffer,
    ReadFramebuffer,
    DrawFramebuffer
};

enum class FramebufferAttachment {
    None,
    Color0,
    Color1,
    Color2,
    Color3,
    Depth,
    Stencil,
    DepthStencil
};

enum class MemoryBarrierBit {
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

inline MemoryBarrierBit operator|(MemoryBarrierBit a, MemoryBarrierBit b) {
    return static_cast<MemoryBarrierBit>(static_cast<int>(a) | static_cast<int>(b));
}

enum class FramebufferStatus {
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

// --- Query Types ---

enum class QueryType {
    SamplesPassed,
    AnySamplesPassed,
    AnySamplesPassedConservative,
    TimeElapsed,
    Timestamp,
    PrimitivesGenerated,
    TransformFeedbackPrimitivesWritten
};

// --- GPU Data Structs ---

struct GpuHandle {
    uint32_t id = 0;
    bool IsValid() const { return id != 0; }
    void Reset() { id = 0; }
    bool operator==(const GpuHandle& other) const { return id == other.id; }
    bool operator!=(const GpuHandle& other) const { return id != other.id; }
};

struct GPUCameraData {
    float projection[16]; // Mat4
    float view[16];       // Mat4
    float viewPos[3];
    float pad0;
};

struct GPUGlobalLightData {
    float lightSpaceMatricesDir[16 * 2];  // MAX_DIR_LIGHTS_SHADOW = 2
    float lightSpaceMatricesSpot[16 * 2]; // MAX_SPOT_LIGHTS_SHADOW = 2
    int numDirLights;
    int nrPointLights;
    int nrSpotLights;
    int u_ReceiveShadow;
    float farPlanePoint;
    float farPlaneSpot;
    float pad0;
    float pad1;
};

// --- Vertex & Texture ---

constexpr int MAX_BONE_INFLUENCE = 4;

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
    unsigned int id = 0;
    std::string type;
    std::string path;

    unsigned char* pixelData = nullptr;
    int width = 0, height = 0, nrComponents = 0;
};

// --- Animation Data ---

#include <render/logic/animdata.h>

struct AssimpNodeData {
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;

    Bone* cachedBone = nullptr;
};

// --- GPU Light Structs ---

struct GPUDirLight {
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

struct GPUPointLight {
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

struct GPUSpotLight {
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
    float pad2;
    float pad3;
    float pad4;
    glm::vec3 ambient;
    float pad5;
    glm::vec3 diffuse;
    float pad6;
    glm::vec3 specular;
    float pad7;
};

// --- Resource Wrappers ---

class GPUFramebuffer {
public:
    GPUFramebuffer(IGraphicsContext& context, uint32_t handleId)
        : m_Context(context) { m_Handle.id = handleId; }

    ~GPUFramebuffer();

    GPUFramebuffer(const GPUFramebuffer&) = delete;
    GPUFramebuffer& operator=(const GPUFramebuffer&) = delete;

    GPUFramebuffer(GPUFramebuffer&& other) noexcept
        : m_Context(other.m_Context), m_Handle(other.m_Handle) {
        other.m_Handle.Reset();
    }
    GPUFramebuffer& operator=(GPUFramebuffer&& other) noexcept;

    uint32_t Get() const { return m_Handle.id; }
    void Release() { m_Handle.Reset(); }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

class GPUSSBO {
public:
    GPUSSBO(IGraphicsContext& context, uint32_t handleId)
        : m_Context(context) { m_Handle.id = handleId; }

    ~GPUSSBO();

    GPUSSBO(const GPUSSBO&) = delete;
    GPUSSBO& operator=(const GPUSSBO&) = delete;

    GPUSSBO(GPUSSBO&& other) noexcept
        : m_Context(other.m_Context), m_Handle(other.m_Handle) {
        other.m_Handle.Reset();
    }
    GPUSSBO& operator=(GPUSSBO&& other) noexcept;

    uint32_t Get() const { return m_Handle.id; }
    void Release() { m_Handle.Reset(); }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

class GPUTexture {
public:
    GPUTexture(IGraphicsContext& context, uint32_t handleId)
        : m_Context(context) { m_Handle.id = handleId; }

    ~GPUTexture();

    GPUTexture(const GPUTexture&) = delete;
    GPUTexture& operator=(const GPUTexture&) = delete;

    GPUTexture(GPUTexture&& other) noexcept
        : m_Context(other.m_Context), m_Handle(other.m_Handle) {
        other.m_Handle.Reset();
    }
    GPUTexture& operator=(GPUTexture&& other) noexcept;

    uint32_t Get() const { return m_Handle.id; }
    void Release() { m_Handle.Reset(); }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};

class GPUUBO {
public:
    GPUUBO(IGraphicsContext& context, uint32_t handleId)
        : m_Context(context) { m_Handle.id = handleId; }

    ~GPUUBO();

    GPUUBO(const GPUUBO&) = delete;
    GPUUBO& operator=(const GPUUBO&) = delete;

    GPUUBO(GPUUBO&& other) noexcept
        : m_Context(other.m_Context), m_Handle(other.m_Handle) {
        other.m_Handle.Reset();
    }
    GPUUBO& operator=(GPUUBO&& other) noexcept;

    uint32_t Get() const { return m_Handle.id; }
    void Release() { m_Handle.Reset(); }

private:
    IGraphicsContext& m_Context;
    GpuHandle m_Handle;
};
