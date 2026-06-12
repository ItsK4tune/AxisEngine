param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
)

$files = Get-ChildItem -Path $Root -Recurse -File -Include *.h,*.hpp,*.hh,*.c,*.cc,*.cpp,*.cxx |
    Where-Object {
        $path = $_.FullName.Replace('\', '/')
        $path -notmatch '/build[^/]*/' -and
        $path -notmatch '/.git/' -and
        $path -notmatch '/third_party/' -and
        $path -notmatch '/external/'
    }

$rules = @(
    @{
        Name = 'OpenGL'
        Pattern = '(#include\s*<glad/glad\.h>|#include\s*<GL/|(?<![A-Za-z0-9_])gl(?:ActiveTexture|AttachShader|BeginQuery|BindBuffer|BindTexture|BlendFunc|BufferData|Clear|CompileShader|CreateProgram|CreateShader|DeleteBuffers|DeleteQueries|DeleteTextures|DispatchCompute|DrawArrays|DrawElements|EnableVertexAttribArray|EndQuery|GenBuffers|GenQueries|GetIntegerv|GetQueryObjectiv|GetQueryObjectuiv|GetQueryObjectui64v|GetString|GetStringi|LinkProgram|ShaderSource|TexImage[123]D?|TexParameter[fiuv]|Uniform(?:1|2|3|4)|UseProgram|VertexAttribPointer|Viewport)\w*\b)'
        Allowed = @('/src/render/strategy/opengl/', '/include/engine/render/strategy/opengl/')
    }
    @{
        Name = 'Vulkan'
        Pattern = '(#include\s*<vulkan/|(?<![A-Za-z0-9_])vk[A-Z]\w*\b)'
        Allowed = @('/src/render/strategy/vulkan/', '/include/engine/render/strategy/vulkan/')
    }
    @{
        Name = 'DirectX'
        Pattern = '(#include\s*<dxgi|#include\s*<d3d12|#include\s*<wrl/|(?<![A-Za-z0-9_])(D3D12|DXGI|IDXGI|ID3D12)\w*\b)'
        Allowed = @('/src/render/strategy/directx/', '/include/engine/render/strategy/directx/', '/sample/')
    }
    @{
        Name = 'GLFW'
        Pattern = '(#include\s*<GLFW/|(?<![A-Za-z0-9_])GLFWwindow\b|(?<![A-Za-z0-9_])glfw(?:Init|Terminate|PollEvents|SwapBuffers|CreateWindow|DestroyWindow|WindowHint|MakeContextCurrent|SetWindowShouldClose|WindowShouldClose|GetKey|GetMouseButton|GetCursorPos|SetCursorPos|SetFramebufferSizeCallback|SetKeyCallback|SetMouseButtonCallback|SetCursorPosCallback|SetScrollCallback|CreateWindowSurface|GetWin32Window)\w*\b)'
        Allowed = @('/src/platform/strategy/glfw/', '/include/engine/platform/strategy/glfw/', '/src/render/strategy/opengl/', '/include/engine/render/strategy/opengl/', '/src/render/strategy/vulkan/', '/src/render/strategy/directx/')
    }
    @{
        Name = 'Bullet'
        Pattern = '(#include\s*<Bullet|#include\s*<bt|(?<![A-Za-z0-9_])bt(?:IDebugDraw|Vector3|Scalar|Collision|RigidBody|CharacterController|Constraint|Dynamics|Ghost|Broadphase|Dispatcher|TypedConstraint|PersistentManifold|MotionState|Quaternion|Transform)\w*\b)'
        Allowed = @('/src/physics/strategy/bullet/', '/include/engine/physics/strategy/bullet/')
    }
    @{
        Name = 'PhysX'
        Pattern = '(#include\s*<Px|(?<![A-Za-z0-9_])Px[A-Z]\w*\b)'
        Allowed = @('/src/physics/strategy/physx/', '/include/engine/physics/strategy/physx/')
    }
    @{
        Name = 'irrKlang'
        Pattern = '(#include\s*<irrKlang|(?<![A-Za-z0-9_])irrklang::\w+|#include\s*<audio/strategy/irrklang/)'
        Allowed = @('/src/audio/strategy/irrklang/', '/include/engine/audio/strategy/irrklang/')
    }
    @{
        Name = 'FMOD'
        Pattern = '(#include\s*<fmod|(?<![A-Za-z0-9_])FMOD::\w+|#include\s*<audio/strategy/fmod/)'
        Allowed = @('/src/audio/strategy/fmod/', '/include/engine/audio/strategy/fmod/')
    }
)

$violations = New-Object System.Collections.Generic.List[string]

foreach ($file in $files) {
    $path = $file.FullName.Replace('\', '/')
    $content = Get-Content -LiteralPath $file.FullName
    for ($lineIndex = 0; $lineIndex -lt $content.Count; ++$lineIndex) {
        $line = $content[$lineIndex]
        foreach ($rule in $rules) {
            $isAllowed = $false
            foreach ($allowedPath in $rule.Allowed) {
                if ($path.Contains($allowedPath)) {
                    $isAllowed = $true
                    break
                }
            }
            if ($line -match $rule.Pattern -and -not $isAllowed) {
                $violations.Add("$($file.FullName):$($lineIndex + 1): $($rule.Name)")
                break
            }
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Host 'Backend boundary violations found:'
    $violations | Sort-Object | ForEach-Object { Write-Host $_ }
    exit 1
}

Write-Host 'Backend boundary check passed.'
