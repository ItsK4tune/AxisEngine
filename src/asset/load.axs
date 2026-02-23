axis_scene:
  Resources:
    Shader:
      Name: phongLitNoShadowShader
      VS: src/asset/shaders/phong_lit_no_shadow.vs
      FS: src/asset/shaders/phong_lit_no_shadow.fs
    Shader:
      Name: phongLitShadowShader
      VS: src/asset/shaders/phong_lit_shadow.vs
      FS: src/asset/shaders/phong_lit_shadow.fs
    Shader:
      Name: pbrLitNoShadowShader
      VS: src/asset/shaders/pbr_lit_no_shadow.vs
      FS: src/asset/shaders/pbr_lit_no_shadow.fs
    Shader:
      Name: pbrLitShadowShader
      VS: src/asset/shaders/pbr_lit_shadow.vs
      FS: src/asset/shaders/pbr_lit_shadow.fs
    Shader:
      Name: unlitShader
      VS: src/asset/shaders/unlit.vs
      FS: src/asset/shaders/unlit.fs
    Shader:
      Name: uiShader
      VS: src/asset/shaders/ui.vs
      FS: src/asset/shaders/ui.fs
    Shader:
      Name: textShader
      VS: src/asset/shaders/text.vs
      FS: src/asset/shaders/text.fs
    Shader:
      Name: particle
      VS: src/asset/shaders/particle.vs
      FS: src/asset/shaders/particle.fs
    Shader:
      Name: videomapShader
      VS: src/asset/shaders/videomap.vs
      FS: src/asset/shaders/videomap.fs
    Shader:
      Name: skyboxShader
      VS: src/asset/shaders/skybox.vs
      FS: src/asset/shaders/skybox.fs

    Font:
      Name: time
      Path: src/asset/fonts/time.ttf
      Size: 16

    Model:
      Name: planeModel
      Path: src/asset/objects/plane/plane.fbx
      Static: 1
    Model:
      Name: planeVideoModel
      Path: src/asset/objects/plane/plane.fbx
      Static: 1
    Model:
      Name: dummyModel
      Path: src/asset/objects/dummy/dummy.fbx
      Static: 1

    Skybox:
      Name: defaultSkybox
      Right: src/asset/skybox/default_right.png
      Left: src/asset/skybox/default_left.png
      Top: src/asset/skybox/default_top.png
      Bottom: src/asset/skybox/default_bottom.png
      Front: src/asset/skybox/default_front.png
      Back: src/asset/skybox/default_back.png

  Entities:
    Skybox:
      Component: SkyboxRenderer
        Skybox: defaultSkybox
        Shader: skyboxShader
