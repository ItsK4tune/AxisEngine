axis_scene:
  Resources:
  Entities:
    S25Random_5:
      Tag: Default
      Component: Transform
        Position: -15.300000 1.050000 7.600000
        Rotation: 180.000000 84.000015 180.000000
        Scale: 2.100000 2.100000 2.100000
      Component: Renderer
        Model: cubeModel
        Shader: deferred_lit
        Order: 0
        CastShadow: true
        ReceiveShadow: true
        IgnoreDepth: false
        Color: 0.570000 0.590000 0.880000 1.000000
        RenderMode: 0
      Component: Material
        Opacity: 1.000000
        Roughness: 0.500000
        Metallic: 0.100000
        AO: 1.000000
        AlphaCutoff: 0.500000
        Emission: 0.000000 0.000000 0.000000
        UVScale: 1.000000 1.000000
        UVOffset: 0.000000 0.000000
    S25Random_4:
      Tag: Default
      Component: Transform
        Position: 6.200000 0.400000 15.800000
        Rotation: 0.000000 -8.999993 0.000000
        Scale: 0.800000 0.800000 0.800000
      Component: Renderer
        Model: sphereModel
        Shader: deferred_lit
        Order: 0
        CastShadow: true
        ReceiveShadow: true
        IgnoreDepth: false
        Color: 0.930000 0.830000 0.090000 1.000000
        RenderMode: 0
      Component: Material
        Opacity: 1.000000
        Roughness: 0.500000
        Metallic: 0.100000
        AO: 1.000000
        AlphaCutoff: 0.500000
        Emission: 0.000000 0.000000 0.000000
        UVScale: 1.000000 1.000000
        UVOffset: 0.000000 0.000000
    S25Random_3:
      Tag: Default
      Component: Transform
        Position: -4.500000 0.950000 0.900000
        Rotation: 0.000000 58.000000 0.000000
        Scale: 1.900000 1.900000 1.900000
      Component: Renderer
        Model: cubeModel
        Shader: deferred_lit
        Order: 0
        CastShadow: true
        ReceiveShadow: true
        IgnoreDepth: false
        Color: 0.900000 0.440000 0.730000 1.000000
        RenderMode: 0
      Component: Material
        Opacity: 1.000000
        Roughness: 0.500000
        Metallic: 0.100000
        AO: 1.000000
        AlphaCutoff: 0.500000
        Emission: 0.000000 0.000000 0.000000
        UVScale: 1.000000 1.000000
        UVOffset: 0.000000 0.000000
    S25Random_2:
      Tag: Default
      Component: Transform
        Position: -12.100000 1.350000 3.700000
        Rotation: 0.000000 52.000004 0.000000
        Scale: 2.700000 2.700000 2.700000
      Component: Renderer
        Model: sphereModel
        Shader: deferred_lit
        Order: 0
        CastShadow: true
        ReceiveShadow: true
        IgnoreDepth: false
        Color: 0.200000 0.380000 0.750000 1.000000
        RenderMode: 0
      Component: Material
        Opacity: 1.000000
        Roughness: 0.500000
        Metallic: 0.100000
        AO: 1.000000
        AlphaCutoff: 0.500000
        Emission: 0.000000 0.000000 0.000000
        UVScale: 1.000000 1.000000
        UVOffset: 0.000000 0.000000
    MainCamera:
      Tag: Default
      Component: Transform
        Position: 0.000000 15.000000 60.000000
        Rotation: 0.000000 0.000003 0.000000
        Scale: 1.000000 1.000000 1.000000
      Component: Script
        Class: DefaultCameraController
    MainCamera:
      Tag: Default
      Component: Transform
        Position: 0.000000 15.000000 60.000000
        Rotation: 0.000000 0.000003 0.000000
        Scale: 1.000000 1.000000 1.000000
      Component: Script
        Class: DefaultCameraController
