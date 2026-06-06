#include "test_framework.h"
#include "test_support.h"

#include <ecs/unit/core_components.h>
#include <scene/logic/scene_manager.h>

namespace
{
std::filesystem::path WriteSceneFile(const std::string& name, const std::string& entityName)
{
    return axis_test_support::WriteTempFile(
        name,
        "axis_scene:\n"
        "  Entities:\n"
        "    " +
            entityName +
            ":\n"
            "      Component: Transform\n");
}
}  // namespace

AXIS_TEST_CASE("SceneManager LoadScene ignores duplicate load")
{
    axis_test_support::SceneServiceFixture fixture;
    SceneManager manager;
    auto path = WriteSceneFile("sm_duplicate.axs", "Root");

    manager.LoadScene(path.string());
    manager.LoadScene(path.string());

    AXIS_CHECK(manager.GetSceneCount() == 1);
    AXIS_CHECK(manager.IsLoaded(path.string()));
}

AXIS_TEST_CASE("SceneManager ClearAllScenes preserves persistent scenes")
{
    axis_test_support::SceneServiceFixture fixture;
    SceneManager manager;
    auto persistentPath = WriteSceneFile("sm_persistent.axs", "PersistentRoot");
    auto transientPath = WriteSceneFile("sm_transient.axs", "TransientRoot");

    manager.LoadScene(persistentPath.string(), true);
    manager.LoadScene(transientPath.string(), false);
    manager.ClearAllScenes();

    AXIS_CHECK(manager.GetSceneCount() == 1);
    AXIS_CHECK(manager.GetScene(persistentPath.string()) != nullptr);
    AXIS_CHECK(manager.GetScene(transientPath.string()) == nullptr);
}

AXIS_TEST_CASE("SceneManager queued scene operations execute FIFO")
{
    axis_test_support::SceneServiceFixture fixture;
    SceneManager manager;
    auto pathA = WriteSceneFile("sm_queue_a.axs", "RootA");
    auto pathB = WriteSceneFile("sm_queue_b.axs", "RootB");

    manager.QueueLoadScene(pathA.string());
    manager.QueueLoadScene(pathB.string());
    manager.QueuePopScene();
    manager.UpdatePendingScene();

    AXIS_CHECK(manager.GetSceneCount() == 1);
    AXIS_CHECK(manager.GetScene(pathA.string()) != nullptr);
    AXIS_CHECK(manager.GetScene(pathB.string()) == nullptr);
}

AXIS_TEST_CASE("SceneManager SetSceneActive propagates inactive state to children")
{
    axis_test_support::SceneServiceFixture fixture;
    SceneManager manager;
    auto parent = fixture.scene.CreateEntity("Parent");
    auto child = fixture.scene.CreateEntity("Child");
    fixture.scene.SetParent(child, parent);
    manager.AddEntity(parent, "arena");

    manager.SetSceneActive("arena", false, fixture.scene);

    AXIS_CHECK(!fixture.scene.GetComponent<InfoComponent>(parent).isActive);
    AXIS_CHECK(!fixture.scene.GetComponent<InfoComponent>(child).isActive);
}
