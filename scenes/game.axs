axis_scene:
  Config:
    SHADOWS: 2
    SHADOW_SIZE: 100.0
    INSTANCING: 1
    CULL_FACE: 1 BACK
    OCCLUSION_CULLING: 1
    DEPTH_TEST: 1 LESS
    ANTIALIASING: TAA
    FRUSTUM: 1
    RENDER_ORDER: 1
    DISTANCE: 100.0
    SHADOW_FRUSTUM: 1
    SHADOW_DISTANCE: 100.0
    WINDOW_WIDTH: 800
    WINDOW_HEIGHT: 600
    WINDOW_MODE: WINDOWED
    WINDOW_MONITOR: 0
    WINDOW_REFRESH_RATE: 0
    VSYNC: 0
    FPS: 0
    PHYSICS_MODE: FAST
    PHYSICS_ASYNC: TRUE

  Entities:
    MainCamera:
      Tag: MainCamera
      Component: Transform
        Position: 0.0 4.0 10.0
        Rotation: 0.0 0.0 0.0
        Scale: 1.0 1.0 1.0
      Component: Camera
        Primary: 1
        FOV: 90.0
        Yaw: -90.0
        Pitch: 0.0
        Near: 0.1
        Far: 1000.0
      Component: Script
        Class: CameraController
        
    Plane:
      Tag: default
      Component: Transform
        Position: 0.0 0.0 0.0
        Rotation: 0.0 0.0 0.0
        Scale: 1.0 1.0 1.0
      Component: Renderer
        Model: planeModel
        Shader: phongLitShadowShader
      Component: Material
        Type: PHONG
        Shininess: 32
        Specular: 0.5 0.5 0.5
      Component: RigidBody
        Type: BOX
        Size: 50.0 0.1 50.0
        Mass: 0.0
        BodyType: STATIC
        Restitution: 1.0
        
    Dummy:
      Layer: 1
      Tag: default
      Component: Transform
        Position: 0.0 10.0 0.0
        Rotation: 0.0 0.0 0.0
        Scale: 0.01 0.01 0.01
      Component: Renderer
        Model: capsuleModel
        Shader: phongLitNoShadowShader
        Order: 1
      Component: Material
        Type: PHONG
        Shininess: 32
        Specular: 0.5 0.5 0.5
      Component: RigidBody
        Type: CAPSULE
        Radius: 1.0
        Height: 2.0
        Mass: 1.0
        BodyType: DYNAMIC
        Offset: 0.0 0.0 0.0

    Dummy2:
      Tag: default
      Component: Transform
        Position: 4.0 10.0 0.0
        Rotation: 0.0 0.0 0.0
        Scale: 0.01 0.01 0.01
      Component: Renderer
        Model: capsuleSmoothModel
        Shader: phongLitNoShadowShader
        Order: 2
      Component: Material
        Type: PHONG
        Shininess: 32
        Specular: 0.5 0.5 0.5
      Component: LOD
        Models: capsuleModel planeModel
        Distances: 20.0 40.0
      Component: RigidBody
        Type: CAPSULE
        Radius: 1.0
        Height: 1.8
        Mass: 1.0
        BodyType: STATIC
        Offset: 0.0 2.0 0.0
        
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
