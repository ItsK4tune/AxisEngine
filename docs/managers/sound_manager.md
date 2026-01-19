# SoundManager API

**Include:** `<engine/core/sound_manager.h>`

The `SoundManager` handles 2D and 3D audio playback using the irrKlang engine.

## Access
Accessible via `Application::GetSoundManager()` or within scripts as `m_App->GetSoundManager()`.

## Public Methods

### Playback
*   `void Play2D(const char* path, bool loop = false)`
    *   Plays a sound file without 3D positioning.
*   `ISound* Play2D(ISoundSource* source, bool loop = false)`
    *   Plays a pre-loaded sound source.
*   `ISound* Play3D(ISoundSource* source, glm::vec3 pos, bool loop = false)`
    *   Plays a sound at a specific 3D location.

### Control
*   `void StopAll()`
    *   Stops all currently playing sounds.
*   `void Stop(ISoundSource* source)`
    *   Stops instances of a specific sound source.
*   `void SetVolume(float volume)`
    *   Sets the global master volume (0.0 to 1.0).
*   `void SetVolume(ISoundSource* source, float volume)`
    *   Sets volume for a specific source.

### 3D Listener
*   `void UpdateListener(glm::vec3 position, glm::vec3 lookDir, glm::vec3 up)`
    *   Updates the "ears" of the listener (usually checking Camera position).

### Advanced
*   `ISoundEngine* GetEngine()`
    *   Returns the raw `irrklang::ISoundEngine*` pointer for direct API access.
