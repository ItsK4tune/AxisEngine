Entities:
  LampRoot:
    Tag: FragmentRoot
    Component: LightPoint
      Color: 1 1 1
      Intensity: 5.0
      Radius: 10.0
    Component: Transform
      Position: 0 0 0
  LampBody:
    Parent: LampRoot
    Tag: FragmentMesh
    Component: Renderer
      Model: cube
      Shader: deferred_lit
      Color: 0.5 0.5 0.5 1.0
    Component: Transform
      Scale: 0.2 1.0 0.2
