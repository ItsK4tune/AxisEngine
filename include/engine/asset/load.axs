axis_scene:
  Resources:
    Shader:
      Name: forward_pbr_lit
      Vertex: include/engine/asset/shaders/opengl/forward_pbr_lit.vs
      Fragment: include/engine/asset/shaders/opengl/forward_pbr_lit.fs
    Shader:
      Name: forward_pbr_lit_shadow
      Vertex: include/engine/asset/shaders/opengl/forward_pbr_lit_shadow.vs
      Fragment: include/engine/asset/shaders/opengl/forward_pbr_lit_shadow.fs
    Shader:
      Name: forward_unlit
      Vertex: include/engine/asset/shaders/opengl/forward_unlit.vs
      Fragment: include/engine/asset/shaders/opengl/forward_unlit.fs
    Shader:
      Name: uiShader
      Vertex: include/engine/asset/shaders/opengl/ui.vs
      Fragment: include/engine/asset/shaders/opengl/ui.fs
    Shader:
      Name: textShader
      Vertex: include/engine/asset/shaders/opengl/text.vs
      Fragment: include/engine/asset/shaders/opengl/text.fs
    Shader:
      Name: particle
      Vertex: include/engine/asset/shaders/opengl/forward_particle.vs
      Fragment: include/engine/asset/shaders/opengl/forward_particle.fs
    Shader:
      Name: videomapShader
      Vertex: include/engine/asset/shaders/opengl/videomap.vs
      Fragment: include/engine/asset/shaders/opengl/videomap.fs
    Shader:
      Name: skyboxShader
      Vertex: include/engine/asset/shaders/opengl/skybox.vs
      Fragment: include/engine/asset/shaders/opengl/skybox.fs
    Shader:
      Name: deferred_lit
      Vertex: include/engine/asset/shaders/opengl/deferred_lit.vs
      Fragment: include/engine/asset/shaders/opengl/deferred_lit.fs
    Shader:
      Name: deferred_lit_shadow
      Vertex: include/engine/asset/shaders/opengl/deferred_lit.vs
      Fragment: include/engine/asset/shaders/opengl/deferred_lit_shadow.fs
    Shader:
      Name: deferred_unlit
      Vertex: include/engine/asset/shaders/opengl/deferred_unlit.vs
      Fragment: include/engine/asset/shaders/opengl/deferred_unlit.fs
    Shader:
      Name: deferred_decal
      Vertex: include/engine/asset/shaders/opengl/deferred_decal.vs
      Fragment: include/engine/asset/shaders/opengl/deferred_decal.fs
    Shader:
      Name: forward_decal
      Vertex: include/engine/asset/shaders/opengl/forward_decal.vs
      Fragment: include/engine/asset/shaders/opengl/forward_decal.fs
    Shader:
      Name: deferred_light
      Vertex: include/engine/asset/shaders/opengl/fxaa.vs
      Fragment: include/engine/asset/shaders/opengl/deferred_light.fs
    Shader:
      Name: shadow_depth
      Vertex: include/engine/asset/shaders/opengl/shadow_depth.vs
      Fragment: include/engine/asset/shaders/opengl/shadow_depth.fs
    Shader:
      Name: shadow_point
      Vertex: include/engine/asset/shaders/opengl/shadow_point.vs
      Fragment: include/engine/asset/shaders/opengl/shadow_point.fs
      Geometry: include/engine/asset/shaders/opengl/shadow_point.gs
    Shader:
      Name: shadow_spot
      Vertex: include/engine/asset/shaders/opengl/shadow_spot.vs
      Fragment: include/engine/asset/shaders/opengl/shadow_spot.fs
    Shader:
      Name: fxaa
      Vertex: include/engine/asset/shaders/opengl/fxaa.vs
      Fragment: include/engine/asset/shaders/opengl/fxaa.fs
    Shader:
      Name: taa
      Vertex: include/engine/asset/shaders/opengl/taa.vs
      Fragment: include/engine/asset/shaders/opengl/taa.fs
    Shader:
      Name: bloom_down
      Vertex: include/engine/asset/shaders/opengl/fxaa.vs
      Fragment: include/engine/asset/shaders/opengl/bloom_downsample.fs
    Shader:
      Name: bloom_up
      Vertex: include/engine/asset/shaders/opengl/fxaa.vs
      Fragment: include/engine/asset/shaders/opengl/bloom_upsample.fs
    Shader:
      Name: hdr_final
      Vertex: include/engine/asset/shaders/opengl/fxaa.vs
      Fragment: include/engine/asset/shaders/opengl/hdr_final.fs
    Shader:
      Name: debug_text
      Vertex: include/engine/asset/shaders/opengl/text.vs
      Fragment: include/engine/asset/shaders/opengl/text.fs
    Shader:
      Name: occlusion
      Vertex: include/engine/asset/shaders/opengl/occlusion_query.vs
      Fragment: include/engine/asset/shaders/opengl/occlusion_query.fs
    Shader:
      Name: terrain
      Vertex: include/engine/asset/shaders/opengl/forward_terrain.vs
      Fragment: include/engine/asset/shaders/opengl/forward_terrain.fs
    Shader:
      Name: terrain_gbuffer
      Vertex: include/engine/asset/shaders/opengl/deferred_terrain.vs
      Fragment: include/engine/asset/shaders/opengl/deferred_terrain.fs
    Shader:
      Name: debug_line
      Vertex: include/engine/asset/shaders/opengl/debug_line.vs
      Fragment: include/engine/asset/shaders/opengl/debug_line.fs
    Shader:
      Name: vignette
      Vertex: include/engine/asset/shaders/opengl/fxaa.vs
      Fragment: include/engine/asset/shaders/opengl/vignette.fs
    Shader:
      Name: glitch
      Vertex: include/engine/asset/shaders/opengl/fxaa.vs
      Fragment: include/engine/asset/shaders/opengl/glitch.fs
    Shader:
      Name: film_grain
      Vertex: include/engine/asset/shaders/opengl/fxaa.vs
      Fragment: include/engine/asset/shaders/opengl/film_grain.fs
    Shader:
      Name: grayscale
      Vertex: include/engine/asset/shaders/opengl/fxaa.vs
      Fragment: include/engine/asset/shaders/opengl/grayscale.fs
    Shader:
      Name: dither
      Vertex: include/engine/asset/shaders/opengl/fxaa.vs
      Fragment: include/engine/asset/shaders/opengl/dither.fs
    Shader:
      Name: error_forward
      Vertex: include/engine/asset/shaders/opengl/forward_unlit.vs
      Fragment: include/engine/asset/shaders/opengl/error_forward.fs
    Shader:
      Name: error_deferred
      Vertex: include/engine/asset/shaders/opengl/deferred_unlit.vs
      Fragment: include/engine/asset/shaders/opengl/error_deferred.fs
    Shader:
      Name: forward_reflect
      Vertex: include/engine/asset/shaders/opengl/forward_pbr_lit.vs
      Fragment: include/engine/asset/shaders/opengl/forward_reflect.fs
    Shader:
      Name: forward_reflect_shadow
      Vertex: include/engine/asset/shaders/opengl/forward_pbr_lit_shadow.vs
      Fragment: include/engine/asset/shaders/opengl/forward_reflect_shadow.fs
    Shader:
      Name: deferred_reflect
      Vertex: include/engine/asset/shaders/opengl/deferred_lit.vs
      Fragment: include/engine/asset/shaders/opengl/deferred_reflect.fs
    Shader:
      Name: forward_transparent
      Vertex: include/engine/asset/shaders/opengl/forward_pbr_lit.vs
      Fragment: include/engine/asset/shaders/opengl/transparent.fs
    Shader:
      Name: deferred_transparent
      Vertex: include/engine/asset/shaders/opengl/deferred_lit.vs
      Fragment: include/engine/asset/shaders/opengl/transparent.fs
    Shader:
      Name: forward_pbr_env
      Vertex: include/engine/asset/shaders/opengl/forward_pbr_lit.vs
      Fragment: include/engine/asset/shaders/opengl/forward_pbr_env.fs
    Texture:
      Name: error_texture
      Path: include/engine/asset/textures/error_checkerboard.tga
    Font:
      Name: time
      Path: include/engine/asset/fonts/time.ttf
      Size: 16
    Font:
      Name: debug_font
      Path: include/engine/asset/fonts/time.ttf
      Size: 24
    Model:
      Name: planeModel
      Path: include/engine/asset/objects/plane/plane.fbx
      Static: 1
    Model:
      Name: capsuleModel
      Path: include/engine/asset/objects/capsule/capsule.fbx
      Static: 1
    Model:
      Name: capsuleSmoothModel
      Path: include/engine/asset/objects/capsule/capsule_smooth.fbx
      Static: 1
    Model:
      Name: cubeModel
      Path: include/engine/asset/objects/cube/cube.fbx
      Static: 1
    Model:
      Name: cylinderModel
      Path: include/engine/asset/objects/cylinder/cylinder.fbx
      Static: 1
    Model:
      Name: sphereModel
      Path: include/engine/asset/objects/sphere/sphere.fbx
      Static: 1
    Skybox:
      Name: defaultSkybox
      Right: include/engine/asset/skyboxs/default_right.png
      Left: include/engine/asset/skyboxs/default_left.png
      Front: include/engine/asset/skyboxs/default_front.png
      Top: include/engine/asset/skyboxs/default_top.png
      Bottom: include/engine/asset/skyboxs/default_bottom.png
      Back: include/engine/asset/skyboxs/default_back.png
  Entities:
    Skybox:
      Component: Transform
        Position: 0.000000 0.000000 0.000000
        Rotation: 0.000000 -0.000000 0.000000
        Scale: 1.000000 1.000000 1.000000
      Component: SkyboxRenderer
        Skybox: defaultSkybox
        Shader: skyboxShader
