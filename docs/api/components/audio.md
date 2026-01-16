# Audio Components

## AudioSourceComponent
**Struct:** `AudioSourceComponent`

An audio source localized in 3D space (or 2D global).

*   `std::string filePath`: Path to sound file.
*   `float volume`: Playback volume (0.0 - 1.0).
*   `bool loop`: Should the sound repeat?
*   `bool is3D`: If true, sound pans/fades based on distance to listener.
*   `bool playOnAwake`: Auto-play when scene starts.
*   `float minDistance`: Distance where sound is at max volume.

**Internal:**
*   `irrklang::ISound* sound`: Pointer to the playing sound instance.
