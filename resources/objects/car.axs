Entities:
  CarRoot:
    Tag: car
    Component: Transform
      Position: 0 0 0

  Body:
    Parent: CarRoot
    Component: Transform
      Position: 0 0.6 0
      Scale: 2.2 0.7 4.5
    Component: Renderer
      Model: cubeModel
      Shader: deferred_lit
    Component: Material
      Type: PHONG
      Shininess: 32
      Specular: 0.5 0.5 0.5

  Cabin:
    Parent: Body
    Component: Transform
      Position: 0 0.8 -0.3
      Scale: 0.8 1.0 0.5
    Component: Renderer
      Model: cubeModel
      Shader: deferred_lit
    Component: Material
      Type: PHONG
      Shininess: 32
      Specular: 0.5 0.5 0.5

  WheelFL:
    Parent: CarRoot
    Component: Transform
      Position: -1.2 0.4 1.5
      Rotation: 0 0 90
      Scale: 0.8 0.3 0.8
    Component: Renderer
      Model: cylinderModel
      Shader: deferred_lit
    Component: Material
      Type: PHONG
      Shininess: 32
      Specular: 0.5 0.5 0.5

  WheelFR:
    Parent: CarRoot
    Component: Transform
      Position: 1.2 0.4 1.5
      Rotation: 0 0 90
      Scale: 0.8 0.3 0.8
    Component: Renderer
      Model: cylinderModel
      Shader: deferred_lit
    Component: Material
      Type: PHONG
      Shininess: 32
      Specular: 0.5 0.5 0.5

  WheelRL:
    Parent: CarRoot
    Component: Transform
      Position: -1.2 0.4 -1.5
      Rotation: 0 0 90
      Scale: 0.8 0.3 0.8
    Component: Renderer
      Model: cylinderModel
      Shader: deferred_lit
    Component: Material
      Type: PHONG
      Shininess: 32
      Specular: 0.5 0.5 0.5

  WheelRR:
    Parent: CarRoot
    Component: Transform
      Position: 1.2 0.4 -1.5
      Rotation: 0 0 90
      Scale: 0.8 0.3 0.8
    Component: Renderer
      Model: cylinderModel
      Shader: deferred_lit
      Color: 0.05 0.05 0.05 1.0
    Component: Material
      Type: PHONG
      Shininess: 16
      Specular: 0.2 0.2 0.2
      Ambient: 1.0 1.0 1.0
