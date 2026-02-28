axis_scene:
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

  Entities:
    Woman:
      Tag: default
      Component: Transform
        Position: 2.0 2.0 2.0
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
        BodyType: STATIC
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
        AmbientStr: 0.1
        DiffuseStr: 0.3
        SpecularStr: 0.3
        
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
        AmbientStr: 0.1
        DiffuseStr: 0.3
        SpecularStr: 0.3
