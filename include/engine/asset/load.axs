axis_scene:
  Resources:
    Shader:
      Name: phongLitNoShadowShader
      vertex: include/engine/asset/shaders/phong_lit_no_shadow.vs
      fragment: include/engine/asset/shaders/phong_lit_no_shadow.fs
    Shader:
      Name: phongLitShadowShader
      vertex: include/engine/asset/shaders/phong_lit_shadow.vs
      fragment: include/engine/asset/shaders/phong_lit_shadow.fs
    Shader:
      Name: pbrLitNoShadowShader
      vertex: include/engine/asset/shaders/pbr_lit_no_shadow.vs
      fragment: include/engine/asset/shaders/pbr_lit_no_shadow.fs
    Shader:
      Name: pbrLitShadowShader
      vertex: include/engine/asset/shaders/pbr_lit_shadow.vs
      fragment: include/engine/asset/shaders/pbr_lit_shadow.fs
    Shader:
      Name: unlitShader
      vertex: include/engine/asset/shaders/unlit.vs
      fragment: include/engine/asset/shaders/unlit.fs
    Shader:
      Name: uiShader
      vertex: include/engine/asset/shaders/ui.vs
      fragment: include/engine/asset/shaders/ui.fs
    Shader:
      Name: textShader
      vertex: include/engine/asset/shaders/text.vs
      fragment: include/engine/asset/shaders/text.fs
    Shader:
      Name: particle
      vertex: include/engine/asset/shaders/particle.vs
      fragment: include/engine/asset/shaders/particle.fs
    Shader:
      Name: videomapShader
      vertex: include/engine/asset/shaders/videomap.vs
      fragment: include/engine/asset/shaders/videomap.fs
    Shader:
      Name: skyboxShader
      vertex: include/engine/asset/shaders/skybox.vs
      fragment: include/engine/asset/shaders/skybox.fs
    Shader:
      Name: gbuffer
      vertex: include/engine/asset/shaders/gbuffer.vs
      fragment: include/engine/asset/shaders/gbuffer.fs
    Shader:
      Name: decal
      vertex: include/engine/asset/shaders/decal.vs
      fragment: include/engine/asset/shaders/decal.fs
    Shader:
      Name: decal_forward
      vertex: include/engine/asset/shaders/decal_forward.vs
      fragment: include/engine/asset/shaders/decal_forward.fs
    Shader:
      Name: deferred_light
      vertex: include/engine/asset/shaders/fxaa.vs
      fragment: include/engine/asset/shaders/deferred_light.fs
    Shader:
      Name: shadow_depth
      vertex: include/engine/asset/shaders/shadow_depth.vs
      fragment: include/engine/asset/shaders/shadow_depth.fs
    Shader:
      Name: shadow_point
      vertex: include/engine/asset/shaders/shadow_point.vs
      fragment: include/engine/asset/shaders/shadow_point.fs
      geometry: include/engine/asset/shaders/shadow_point.gs
    Shader:
      Name: shadow_spot
      vertex: include/engine/asset/shaders/shadow_spot.vs
      fragment: include/engine/asset/shaders/shadow_spot.fs
    Shader:
      Name: fxaa
      vertex: include/engine/asset/shaders/fxaa.vs
      fragment: include/engine/asset/shaders/fxaa.fs
    Shader:
      Name: taa
      vertex: include/engine/asset/shaders/taa.vs
      fragment: include/engine/asset/shaders/taa.fs
    Shader:
      Name: bloom_down
      vertex: include/engine/asset/shaders/fxaa.vs
      fragment: include/engine/asset/shaders/bloom_downsample.fs
    Shader:
      Name: bloom_up
      vertex: include/engine/asset/shaders/fxaa.vs
      fragment: include/engine/asset/shaders/bloom_upsample.fs
    Shader:
      Name: hdr_final
      vertex: include/engine/asset/shaders/fxaa.vs
      fragment: include/engine/asset/shaders/hdr_final.fs
    Shader:
      Name: debug_text
      vertex: include/engine/asset/shaders/text.vs
      fragment: include/engine/asset/shaders/text.fs
    Shader:
      Name: occlusion
      vertex: include/engine/asset/shaders/occlusion_query.vs
      fragment: include/engine/asset/shaders/occlusion_query.fs
    Shader:
      Name: terrain
      vertex: include/engine/asset/shaders/terrain.vs
      fragment: include/engine/asset/shaders/terrain.fs
    Shader:
      Name: terrain_gbuffer
      vertex: include/engine/asset/shaders/terrain_gbuffer.vs
      fragment: include/engine/asset/shaders/terrain_gbuffer.fs
    Shader:
      Name: debug_line
      vertex: include/engine/asset/shaders/debug_line.vs
      fragment: include/engine/asset/shaders/debug_line.fs
    Shader:
      Name: error_shader
      vertex: include/engine/asset/shaders/error_forward.vs
      fragment: include/engine/asset/shaders/error_forward.fs
    
    Texture:
      Name: error_texture
      Path: include/engine/asset/textures/error_checkerboard.ppm

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
      Top: include/engine/asset/skyboxs/default_top.png
      Bottom: include/engine/asset/skyboxs/default_bottom.png
      Front: include/engine/asset/skyboxs/default_front.png
      Back: include/engine/asset/skyboxs/default_back.png

  Entities:
    Skybox:
      Component: SkyboxRenderer
        Skybox: defaultSkybox
        Shader: skyboxShader
