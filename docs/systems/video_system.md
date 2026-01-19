# Video System

The `VideoSystem` manages the playback and rendering of video files (MP4, etc.) using FFmpeg.

## Core Responsibilities
1.  **Decoding**: Uses `VideoDecoder` to decode video frames.
2.  **Synchronization**: Updates video time based on delta time (and playback speed).
3.  **Rendering Sync**: Automatically injects the decoded video frame (OpenGL Texture) into the `UIRendererComponent` (or others) of the same entity.

## Dependencies

*   **FFmpeg**: Requires `avcodec`, `avformat`, `avutil`, `swscale`, `swresample` DLLs and libraries.
*   **OpenGL**: Generates textures using standard GL calls.

## Usage

The system is automatically updated by the active `State`. You do not need to call `Update` manually unless writing a custom State.

```cpp
// In your GameState::OnUpdate
m_App->GetVideoSystem().Update(m_App->GetScene(), dt);
```

To use in a Scene, simply add the `VIDEO_PLAYER` component.

## API References

### VideoDecoder Class (`graphic/video_decoder.h`)

Low-level wrapper around FFmpeg.

*   `bool Load(std::string path)`: Loads video.
*   `void Play()`, `void Pause()`, `void Stop()`: Playback controls.
*   `void Update(float dt)`: Decodes frames up to current timestamp.
*   `unsigned int GetTextureID()`: Returns OpenGL texture ID of current frame.

### VideoPlayerComponent (`ecs/component.h`)

**Data:**
*   `std::string filePath`
*   `bool isPlaying`
*   `bool isLooping`
*   `float speed`

**Methods:**
*   `void Play()`: Resumes playback.
*   `void Pause()`: Pauses playback.
*   `void Stop()`: Stops and resets to beginning.
*   `void Replay()`: Restarts from beginning.
*   `void Seek(double time)`: Jumps to specific time (seconds).
