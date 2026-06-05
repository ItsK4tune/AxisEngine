#include "test_framework.h"

#include <physics/logic/collision_matrix.h>

AXIS_TEST_CASE("CollisionMatrix allows collisions by default")
{
    CollisionMatrix matrix;

    AXIS_CHECK(matrix.IsEmpty());
    AXIS_CHECK(matrix.CanCollide("player", "enemy", "PlayerA", "EnemyA"));
}

AXIS_TEST_CASE("CollisionMatrix ignores tag collisions symmetrically")
{
    CollisionMatrix matrix;
    matrix.IgnoreTagCollision("player", "enemy");

    AXIS_CHECK(!matrix.IsEmpty());
    AXIS_CHECK(!matrix.CanCollide("player", "enemy", "PlayerA", "EnemyA"));
    AXIS_CHECK(!matrix.CanCollide("enemy", "player", "EnemyA", "PlayerA"));
}

AXIS_TEST_CASE("CollisionMatrix ignores name collisions symmetrically")
{
    CollisionMatrix matrix;
    matrix.IgnoreNameCollision("DoorA", "KeyA");

    AXIS_CHECK(!matrix.CanCollide("prop", "item", "DoorA", "KeyA"));
    AXIS_CHECK(!matrix.CanCollide("item", "prop", "KeyA", "DoorA"));
}

AXIS_TEST_CASE("CollisionMatrix reset clears ignored pairs")
{
    CollisionMatrix matrix;
    matrix.IgnoreTagCollision("player", "enemy");
    matrix.IgnoreNameCollision("DoorA", "KeyA");
    matrix.Reset();

    AXIS_CHECK(matrix.IsEmpty());
    AXIS_CHECK(matrix.CanCollide("player", "enemy", "DoorA", "KeyA"));
}
