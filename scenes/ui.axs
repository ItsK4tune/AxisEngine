axis_scene:
  Resources:
    Texture:
      Name: hud_crosshair
      Path: resources/textures/hud_crosshair.png
    Texture:
      Name: hud_panel_bg
      Path: resources/textures/hud_panel_bg.png
    Texture:
      Name: hud_health_icon
      Path: resources/textures/hud_health_icon.png

    Font:
      Name: time-60
      Path: includes/engine/asset/fonts/time.ttf
      Size: 60
      
  Entities:
    # --- CROSSHAIR ---
    Crosshair:
      Component: UITransform
        anchorMin: 0.5 0.5
        anchorMax: 0.5 0.5
        offsetMin: -16 -16
        offsetMax: 16 16
        pivot: 0.5 0.5
        zIndex: 100
      Component: UIRenderer
        texture: hud_crosshair
        color: 1 1 1 1

    # --- HEALTH PANEL (Bottom Left) ---
    HealthPanel:
      Component: UITransform
        anchorMin: 0.0 1.0
        anchorMax: 0.0 1.0
        offsetMin: 20 -80
        offsetMax: 220 -20
        pivot: 0.0 1.0
        zIndex: 10
      Component: UIRenderer
        texture: hud_panel_bg
        color: 0.1 0.1 0.1 0.7
      Component: UIFlex
        direction: Row
        spacing: 10
        padding: 10 5 10 5

    HealthIcon:
      Parent: HealthPanel
      Component: UITransform
        size: 32 32
      Component: UIRenderer
        texture: hud_health_icon
        color: 1 0 0 1

    HealthText:
      Parent: HealthPanel
      Component: UITransform
        size: 100 40
      Component: UIText
        text: 100
        font: time-60
        scale: 1.5
        color: 1 1 1 1
        alignment: Left

    # --- AMMO PANEL (Bottom Right) ---
    AmmoPanel:
      Component: UITransform
        anchorMin: 1.0 1.0
        anchorMax: 1.0 1.0
        offsetMin: -220 -80
        offsetMax: -20 -20
        pivot: 1.0 1.0
        zIndex: 10
      Component: UIRenderer
        texture: hud_panel_bg
        color: 0.1 0.1 0.1 0.7
      Component: UIFlex
        direction: Row
        spacing: 10
        padding: 10 5 10 5

    AmmoText:
      Parent: AmmoPanel
      Component: UITransform
        size: 80 40
      Component: UIText
        text: 30
        font: time-60
        scale: 1.5
        color: 1 0.8 0 1
        alignment: Right

    AmmoSlash:
       Parent: AmmoPanel
       Component: UITransform
         size: 15 40
       Component: UIText
         text: /
         font: time-60
         scale: 1.2
         color: 0.6 0.6 0.6 1

    AmmoReserveText:
      Parent: AmmoPanel
      Component: UITransform
        size: 60 40
      Component: UIText
        text: 90
        font: time-60
        scale: 1.2
        color: 0.8 0.8 0.8 1

    # --- KILLFEED (Top Right) ---
    Killfeed:
      Component: UITransform
        anchorMin: 1.0 0.0
        anchorMax: 1.0 0.0
        offsetMin: -350 20
        offsetMax: -20 220
        pivot: 1.0 0.0
      Component: UIFlex
        direction: Column
        spacing: 5
    
    KillEntry1:
      Parent: Killfeed
      Component: UITransform
        size: 300 25
      Component: UIText
        text: Player1  [AK47]  Enemy1
        font: time-60
        scale: 0.8
        color: 1 1 1 1
        alignment: Right

    KillEntry2:
      Parent: Killfeed
      Component: UITransform
        size: 300 25
      Component: UIText
        text: Player1  [Knife]  Enemy2
        font: time-60
        scale: 0.8
        color: 1 0 0 1
        alignment: Right

    # --- SUBTITLES (Bottom Center) ---
    SubtitleArea:
      Component: UITransform
        anchorMin: 0.5 0.9
        anchorMax: 0.5 0.9
        offsetMin: -400 -100
        offsetMax: 400 0
        pivot: 0.5 1.0
      Component: UIText
        text: Commander: Move to site B and plant the bomb! We are running out of time, stay alert for any enemy rotation from tunnels.
        font: time-60
        scale: 0.9
        color: 1 1 1 0.9
        alignment: Center
        wordWrap: true
        maxWidth: 1000
