#pragma once

#include <ecs/interface/i_update_system.h>
#include <ecs/logic/ecs_command_buffer.h>
#include <ecs/logic/frame_snapshot.h>

class IParallelUpdateSystem : virtual public IUpdateSystem
{
public:
    virtual ~IParallelUpdateSystem() = default;

    virtual void CaptureSnapshot(Scene& scene, FrameSnapshot& snapshot) = 0;
    virtual void UpdateParallel(const FrameSnapshot& snapshot, ECSCommandBuffer& commands, float dt) = 0;
};
