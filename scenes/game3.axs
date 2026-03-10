axis_scene:
  Config:
    LOG_LEVEL: 4
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
    Woman:
      Tag: ally
      Component: Transform
        Position: 2.0 2.0 2.0
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

    Woman2:
      Tag: enemy
      Component: Transform
        Position: 20.0 2.0 2.0
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

    Woman3:
      Tag: ally
      Component: Transform
        Position: 18.0 2.0 2.0
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

    Woman4:
      Tag: enemy
      Component: Transform
        Position: 16.0 2.0 2.0
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

    Woman5:
      Tag: ally
      Component: Transform
        Position: 14.0 2.0 2.0
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

    Woman6:
      Tag: enemy
      Component: Transform
        Position: 12.0 2.0 2.0
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

    Woman7:
      Tag: ally
      Component: Transform
        Position: 10.0 2.0 2.0
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

    Woman8:
      Tag: enemy
      Component: Transform
        Position: 8.0 2.0 2.0
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

    Woman9:
      Tag: ally
      Component: Transform
        Position: 6.0 2.0 2.0
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

    Woman10:
      Tag: enemy
      Component: Transform
        Position: 4.0 2.0 2.0
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

    Floor:
      Tag: Walkable
      Component: Transform
        Position: 0.0 0.0 0.0
        Rotation: 0.0 0.0 0.0
        Scale: 100.0 1.0 100.0
      Component: Renderer
        Model: planeModel
        Shader: unlitShader
      Component: Material
        Type: PHONG
        Shininess: 32
        Specular: 0.5 0.5 0.5
      Component: RigidBody
        Type: BOX
        Size: 100.0 0.1 100.0
        Mass: 0.0
        BodyType: STATIC
        Restitution: 1.0

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
