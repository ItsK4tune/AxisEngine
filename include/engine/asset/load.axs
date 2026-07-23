axis_scene:
  Resources:
    Shader:
      Name: forward_pbr_lit
      Fragment: asset://shaders/forward_pbr_lit.fs
      Vertex: asset://shaders/forward_pbr_lit.vs
    Shader:
      Name: forward_pbr_lit_shadow
      Fragment: asset://shaders/forward_pbr_lit_shadow.fs
      Vertex: asset://shaders/forward_pbr_lit_shadow.vs
    Shader:
      Name: forward_unlit
      Fragment: asset://shaders/forward_unlit.fs
      Vertex: asset://shaders/forward_unlit.vs
    Shader:
      Name: uiShader
      Fragment: asset://shaders/ui.fs
      Vertex: asset://shaders/ui.vs
    Shader:
      Name: textShader
      Fragment: asset://shaders/text.fs
      Vertex: asset://shaders/text.vs
    Shader:
      Name: particle
      Fragment: asset://shaders/forward_particle.fs
      Vertex: asset://shaders/forward_particle.vs
    Shader:
      Name: videomapShader
      Fragment: asset://shaders/videomap.fs
      Vertex: asset://shaders/videomap.vs
    Shader:
      Name: skyboxShader
      Fragment: asset://shaders/skybox.fs
      Vertex: asset://shaders/skybox.vs
    Shader:
      Name: deferred_lit
      Fragment: asset://shaders/deferred_lit.fs
      Vertex: asset://shaders/deferred_lit.vs
    Shader:
      Name: deferred_lit_shadow
      Fragment: asset://shaders/deferred_lit_shadow.fs
      Vertex: asset://shaders/deferred_lit.vs
    Shader:
      Name: deferred_unlit
      Fragment: asset://shaders/deferred_unlit.fs
      Vertex: asset://shaders/deferred_unlit.vs
    Shader:
      Name: deferred_decal
      Fragment: asset://shaders/deferred_decal.fs
      Vertex: asset://shaders/deferred_decal.vs
    Shader:
      Name: forward_decal
      Fragment: asset://shaders/forward_decal.fs
      Vertex: asset://shaders/forward_decal.vs
    Shader:
      Name: deferred_light
      Fragment: asset://shaders/deferred_light.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: shadow_depth
      Fragment: asset://shaders/shadow_depth.fs
      Vertex: asset://shaders/shadow_depth.vs
    Shader:
      Name: shadow_point
      Fragment: asset://shaders/shadow_point.fs
      Geometry: asset://shaders/shadow_point.gs
      Vertex: asset://shaders/shadow_point.vs
    Shader:
      Name: shadow_spot
      Fragment: asset://shaders/shadow_spot.fs
      Vertex: asset://shaders/shadow_spot.vs
    Shader:
      Name: fxaa
      Fragment: asset://shaders/fxaa.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: taa
      Fragment: asset://shaders/taa.fs
      Vertex: asset://shaders/taa.vs
    Shader:
      Name: bloom_down
      Fragment: asset://shaders/bloom_downsample.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: bloom_up
      Fragment: asset://shaders/bloom_upsample.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: hdr_final
      Fragment: asset://shaders/hdr_final.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: debug_text
      Fragment: asset://shaders/text.fs
      Vertex: asset://shaders/text.vs
    Shader:
      Name: editor_selection_outline
      Fragment: asset://shaders/editor_selection_outline.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: occlusion
      Fragment: asset://shaders/occlusion_query.fs
      Vertex: asset://shaders/occlusion_query.vs
    Shader:
      Name: terrain
      Fragment: asset://shaders/forward_terrain.fs
      Vertex: asset://shaders/forward_terrain.vs
    Shader:
      Name: terrain_gbuffer
      Fragment: asset://shaders/deferred_terrain.fs
      Vertex: asset://shaders/deferred_terrain.vs
    Shader:
      Name: debug_line
      Fragment: asset://shaders/debug_line.fs
      Vertex: asset://shaders/debug_line.vs
    Shader:
      Name: vignette
      Fragment: asset://shaders/vignette.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: glitch
      Fragment: asset://shaders/glitch.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: film_grain
      Fragment: asset://shaders/film_grain.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: grayscale
      Fragment: asset://shaders/grayscale.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: dither
      Fragment: asset://shaders/dither.fs
      Vertex: asset://shaders/fxaa.vs
    Shader:
      Name: error_forward
      Fragment: asset://shaders/error_forward.fs
      Vertex: asset://shaders/forward_unlit.vs
    Shader:
      Name: error_deferred
      Fragment: asset://shaders/error_deferred.fs
      Vertex: asset://shaders/deferred_unlit.vs
    Shader:
      Name: forward_reflect
      Fragment: asset://shaders/forward_reflect.fs
      Vertex: asset://shaders/forward_pbr_lit.vs
    Shader:
      Name: forward_reflect_shadow
      Fragment: asset://shaders/forward_reflect_shadow.fs
      Vertex: asset://shaders/forward_pbr_lit_shadow.vs
    Shader:
      Name: deferred_reflect
      Fragment: asset://shaders/deferred_reflect.fs
      Vertex: asset://shaders/deferred_lit.vs
    Shader:
      Name: forward_transparent
      Fragment: asset://shaders/transparent.fs
      Vertex: asset://shaders/forward_pbr_lit.vs
    Shader:
      Name: deferred_transparent
      Fragment: asset://shaders/transparent.fs
      Vertex: asset://shaders/deferred_lit.vs
    Shader:
      Name: forward_pbr_env
      Fragment: asset://shaders/forward_pbr_env.fs
      Vertex: asset://shaders/forward_pbr_lit.vs
    Texture:
      Name: error_texture
      Path: asset://textures/error_checkerboard.tga
    Font:
      Name: time
      Path: asset://fonts/time.ttf
      Size: 64
    Font:
      Name: debug_font
      Path: asset://fonts/time.ttf
      Size: 24
    Model:
      Name: planeModel
      Path: asset://objects/plane/plane.fbx
      Static: 1
    Model:
      Name: capsuleModel
      Path: asset://objects/capsule/capsule.fbx
      Static: 1
    Model:
      Name: capsuleSmoothModel
      Path: asset://objects/capsule/capsule_smooth.fbx
      Static: 1
    Model:
      Name: cubeModel
      Path: asset://objects/cube/cube.fbx
      Static: 1
    Model:
      Name: cylinderModel
      Path: asset://objects/cylinder/cylinder.fbx
      Static: 1
    Model:
      Name: sphereModel
      Path: asset://objects/sphere/sphere.fbx
      Static: 1
    Skybox:
      Name: defaultSkybox
      Back: asset://skyboxs/default_back.png
      Bottom: asset://skyboxs/default_bottom.png
      Front: asset://skyboxs/default_front.png
      Left: asset://skyboxs/default_left.png
      Right: asset://skyboxs/default_right.png
      Top: asset://skyboxs/default_top.png
  Entities:
    Skybox:
      Component: Transform
        Position: 0.000000 0.000000 0.000000
        Rotation: 0.000000 -0.000000 0.000000
        Scale: 1.000000 1.000000 1.000000
      Component: SkyboxRenderer
        Skybox: defaultSkybox
        Shader: skyboxShader
        Primary: true
