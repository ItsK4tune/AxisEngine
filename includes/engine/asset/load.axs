axis_scene:
  Resources:
    Shader:
      Name: phongLitNoShadowShader
      VS: includes/engine/asset/shaders/phong_lit_no_shadow.vs
      FS: includes/engine/asset/shaders/phong_lit_no_shadow.fs
    Shader:
      Name: phongLitShadowShader
      VS: includes/engine/asset/shaders/phong_lit_shadow.vs
      FS: includes/engine/asset/shaders/phong_lit_shadow.fs
    Shader:
      Name: pbrLitNoShadowShader
      VS: includes/engine/asset/shaders/pbr_lit_no_shadow.vs
      FS: includes/engine/asset/shaders/pbr_lit_no_shadow.fs
    Shader:
      Name: pbrLitShadowShader
      VS: includes/engine/asset/shaders/pbr_lit_shadow.vs
      FS: includes/engine/asset/shaders/pbr_lit_shadow.fs
    Shader:
      Name: unlitShader
      VS: includes/engine/asset/shaders/unlit.vs
      FS: includes/engine/asset/shaders/unlit.fs
    Shader:
      Name: uiShader
      VS: includes/engine/asset/shaders/ui.vs
      FS: includes/engine/asset/shaders/ui.fs
    Shader:
      Name: textShader
      VS: includes/engine/asset/shaders/text.vs
      FS: includes/engine/asset/shaders/text.fs
    Shader:
      Name: particle
      VS: includes/engine/asset/shaders/particle.vs
      FS: includes/engine/asset/shaders/particle.fs
    Shader:
      Name: videomapShader
      VS: includes/engine/asset/shaders/videomap.vs
      FS: includes/engine/asset/shaders/videomap.fs
    Shader:
      Name: skyboxShader
      VS: includes/engine/asset/shaders/skybox.vs
      FS: includes/engine/asset/shaders/skybox.fs

    Font:
      Name: time
      Path: includes/engine/asset/fonts/time.ttf
      Size: 16

    Model:
      Name: planeModel
      Path: includes/engine/asset/objects/plane/plane.fbx
      Static: 1
    Model:
      Name: capsuleModel
      Path: includes/engine/asset/objects/capsule/capsule.fbx
      Static: 1
    Model:
      Name: capsuleSmoothModel
      Path: includes/engine/asset/objects/capsule/capsule_smooth.fbx
      Static: 1
    Model:
      Name: cubeModel
      Path: includes/engine/asset/objects/cube/cube.fbx
      Static: 1
    Model:
      Name: cylinderModel
      Path: includes/engine/asset/objects/cylinder/cylinder.fbx
      Static: 1
    Model:
      Name: sphereModel
      Path: includes/engine/asset/objects/sphere/sphere.fbx
      Static: 1

    Skybox:
      Name: defaultSkybox
      Right: includes/engine/asset/skyboxs/default_right.png
      Left: includes/engine/asset/skyboxs/default_left.png
      Top: includes/engine/asset/skyboxs/default_top.png
      Bottom: includes/engine/asset/skyboxs/default_bottom.png
      Front: includes/engine/asset/skyboxs/default_front.png
      Back: includes/engine/asset/skyboxs/default_back.png

  Entities:
    Skybox:
      Component: SkyboxRenderer
        Skybox: defaultSkybox
        Shader: skyboxShader
