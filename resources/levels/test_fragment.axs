Entities:
  MainCamera:
    Tag: MainCamera
    Component: Camera
      Primary: true
      FOV: 60.0
      Near: 0.1
      Far: 1000.0
    Component: Transform
      Position: 0 5 15
      Rotation: -10 0 0

  DefaultLamp:
    Tag: FragmentInstance
    Component: Fragment
      Path: resources/objects/Lamp.axs
    Component: Transform
      Position: -5 0 0

  OverriddenLamp:
    Tag: FragmentInstance
    Component: Fragment
      Path: resources/objects/Lamp.axs
      Override:
        LampRoot:
          Component: LightPoint
            Color: 1.0 0.5 0.0
            Intensity: 20.0
        LampBody:
          Component: Renderer
            Color: 1.0 0.0 0.0 1.0
          Component: Transform
            Scale: 0.5 3.0 0.5
    Component: Transform
      Position: 5 0 0
