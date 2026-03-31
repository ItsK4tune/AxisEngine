axis_scene:
  Config:
    RENDER_PATH: FORWARD
    WINDOW_MODE: WINDOWED

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
    Shader:
      Name: dithering_bw
      vertex: include/engine/asset/shaders/fxaa.vs
      fragment: resources/shaders/dithering_bw.fs

    Audio:
      Name: bgm
      Path: resources/audios/bgm.mp3
      Volume: 1
      Speed: 1.0
    Audio:
      Name: 2dSound
      Path: resources/audios/2dsound.mp3
    Audio:
      Name: 3dSound
      Path: resources/audios/3dsound.mp3
      Is3D: true

  Entities:
    BGM_Player:
      Component: AudioSource
        Audio: bgm
        PlayOnAwake: true
        Loop: true
        Volume: 1
        Pitch: 1.1
        Speed: 1.0
        Is3d: false

    Particle_Demo:
      Component: Transform
        Position: 0.0 10.0 19.5
      Component: ParticleEmitter
        Texture: resources/textures/particle_star.png
        MaxParticles: 500
        Life: 2.0

    Woman_Ally:
      Tag: ally
      Component: Transform
        Position: -5.0 5.0 -5.0
        Rotation: 0.0 0.0 0.0
        Scale: 1 1 1
      Component: Renderer
        Model: womanModel
        Shader: forward_unlit
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

    TransparentSphere:
      Tag: transparent
      Component: Transform
        Position: 0.0 5.0 0.0
        Scale: 2.0 2.0 2.0
      Component: Renderer
        Model: sphereModel
        Shader: forward_phong_lit
      Component: Material
        Type: PHONG
        Opacity: 0.5
        Ambient: 0.0 1.0 1.0

    GlobalPostProcess:
      Component: Transform
        Position: 0 0 0
      Component: PostProcess
        Active: true
        Effects: dithering_bw:300

    ReflectiveSphere:
      Tag: reflective
      Component: Transform
        Position: 3.0 5.0 0.0
        Scale: 2.0 2.0 2.0
      Component: Renderer
        Model: sphereModel
        Shader: forward_phong_lit
      Component: Material
        Type: PHONG
        Ambient: 1.0 1.0 1.0
      Component: ReflectionProbe
        Type: DYNAMIC
        Radius: 20.0
        Resolution: 256
        BoxProjection: true
      Component: Reflective
        Active: true
        Reflectivity: 0.9
        FresnelPower: 5.0
        FresnelBias: 0.05

    ReflectiveCube:
      Tag: reflective
      Component: Transform
        Position: -3.0 5.0 0.0
        Scale: 2.0 2.0 2.0
      Component: Renderer
        Model: cubeModel
        Shader: forward_phong_lit
      Component: Material
        Type: PHONG
        Ambient: 1.0 1.0 1.0
      Component: ReflectionProbe
        Type: DYNAMIC
        Radius: 20.0
        Resolution: 256
        BoxProjection: true
      Component: Reflective
        Active: true
        Reflectivity: 0.8
        FresnelPower: 3.0
        FresnelBias: 0.1

