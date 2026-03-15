axis_scene:
  Config:
    RENDER_PATH: FORWARD

  Resources:
    Model:
      Name: womanModel
      Path: resources/objects/woman/defeated.fbx
      Static: 0

    Animation:
      Name: defeatedAnimation
      Path: resources/objects/woman/defeated.fbx
      Model: womanModel
    Animation:
      Name: spinAnimation
      Path: resources/objects/woman/spin.fbx
      Model: womanModel

    Shader:
      Name: demoPortShader
      vertex: resources/shaders/demo_port.vs
      fragment: resources/shaders/demo_port.fs

  Entities:
    Woman_Ally:
      Tag: ally
      Component: Transform
        Position: -5.0 5.0 -5.0
        Rotation: 0.0 0.0 0.0
        Scale: 1 1 1
      Component: Renderer
        Model: womanModel
        Shader: unlitShader
        Order: 1
      Component: Material
        Type: PHONG
        Shininess: 32
        Specular: 0.5 0.5 0.5
      Component: Animator
        Animation: defeatedAnimation spinAnimation
        Speed: 1.0
        StartTime: 0.0
        Rate: 30.0
      Component: RigidBody
        Type: CAPSULE
        Radius: 1.0
        Height: 2.0
        Mass: 1.0
        BodyType: KINEMATIC
        Offset: 0.0 0.0 0.0

    Woman_Enemy:
      Tag: enemy
      Component: Transform
        Position: 5.0 5.0 5.0
        Rotation: 0.0 0.0 0.0
        Scale: 1 1 1
      Component: Renderer
        Model: womanModel
        Shader: demoPortShader
        Order: 1
      Component: Material
        Type: PHONG
        Shininess: 32
        Specular: 0.5 0.5 0.5
      Component: Animator
        Animation: defeatedAnimation spinAnimation
        Speed: 1.0
        StartTime: 0.0
        Rate: 30.0
      Component: RigidBody
        Type: CAPSULE
        Radius: 1.0
        Height: 2.0
        Mass: 1.0
        BodyType: KINEMATIC
        Offset: 0.0 0.0 0.0

    Sun1_Dir:
      Tag: default
      Component: Transform
        Position: 0.0 10.0 0.0
        Rotation: -45.0 -30.0 0.0
        Scale: 1.0 1.0 1.0
      Component: LightDir
        CastShadow: 1
        Color: 1.0 0.95 0.8
        Intensity: 0.8
        Ambient: 0.1
        Diffuse: 0.3
        Specular: 0.3
      
    Sun2_Dir:
      Tag: default
      Component: Transform
        Position: 0.0 10.0 0.0
        Rotation: -45.0 30.0 0.0
        Scale: 1.0 1.0 1.0
      Component: LightDir
        CastShadow: 1
        Color: 0.9 0.9 1.0
        Intensity: 0.6
        Ambient: 0.1
        Diffuse: 0.3
        Specular: 0.3

    WallTest:
      Tag: wall
      Component: Transform
        Position: 0.0 10.0 20.0
        Rotation: 90.0 0.0 0.0
        Scale: 10.0 10.0 10.0
      Component: Renderer
        Model: planeModel
        Shader: phongLitShadowShader
      Component: Material
        Type: PHONG
        Shininess: 32
        Specular: 0.5 0.5 0.5
      Component: RigidBody
        Type: BOX
        Size: 10.0 10.0 0.1
        Mass: 0.0
        BodyType: STATIC
        Rotation: 90.0 0.0 0.0

