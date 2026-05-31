Entities:
  TreeTrunk:
    Component: Transform
      Position: 0 1.5 0
      Scale: 0.8 3 0.8
    Component: Renderer
      Model: cubeModel
      Shader: deferred_lit
      Color: 0.42 0.24 0.1 1
  TreeFoliage:
    Parent: TreeTrunk
    Component: Transform
      Position: 0 3.5 0
      Scale: 2.5 2.5 2.5
    Component: Renderer
      Model: sphereModel
      Shader: deferred_lit
      Color: 0.12 0.58 0.18 1
