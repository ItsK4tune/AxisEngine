# - Try to find Bullet Physics
# Once done, this will define:
#
#   BULLET_FOUND           - system has Bullet
#   BULLET_INCLUDE_DIR     - the Bullet include directory
#   BULLET_LIBRARIES       - all Bullet libraries
#   BULLET_DYNAMICS_LIB    - BulletDynamics
#   BULLET_COLLISION_LIB   - BulletCollision
#   BULLET_LINEARMATH_LIB  - LinearMath

# --- Include directory ---
FIND_PATH(BULLET_INCLUDE_DIR btBulletDynamicsCommon.h
    /usr/include
    /usr/local/include
    /opt/local/include
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/bullet
)

# --- Library: BulletDynamics ---
FIND_LIBRARY(BULLET_DYNAMICS_LIB BulletDynamics
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /opt/local/lib
    ${CMAKE_SOURCE_DIR}/lib
)

# --- Library: BulletCollision ---
FIND_LIBRARY(BULLET_COLLISION_LIB BulletCollision
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /opt/local/lib
    ${CMAKE_SOURCE_DIR}/lib
)

# --- Library: LinearMath ---
FIND_LIBRARY(BULLET_LINEARMATH_LIB LinearMath
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /opt/local/lib
    ${CMAKE_SOURCE_DIR}/lib
)

# --- Validate ---
IF(BULLET_INCLUDE_DIR AND BULLET_DYNAMICS_LIB AND BULLET_COLLISION_LIB AND BULLET_LINEARMATH_LIB)
    SET(BULLET_FOUND TRUE)
    SET(BULLET_LIBRARIES
        ${BULLET_DYNAMICS_LIB}
        ${BULLET_COLLISION_LIB}
        ${BULLET_LINEARMATH_LIB}
    )
ENDIF()

# --- Messages ---
IF(BULLET_FOUND)
    IF(NOT Bullet_FIND_QUIETLY)
        MESSAGE(STATUS "Found Bullet: ${BULLET_LIBRARIES}")
    ENDIF()
ELSE()
    IF(Bullet_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find Bullet Physics")
    ELSE()
        MESSAGE(STATUS "Bullet not found")
    ENDIF()
ENDIF()
