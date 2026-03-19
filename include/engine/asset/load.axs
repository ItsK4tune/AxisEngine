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

    Font:
      Name: time
      Path: include/engine/asset/fonts/time.ttf
      Size: 16

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
