# - Try to find Bullet Physics
# Once done, this will define:
#
#   BULLET_FOUND           - system has Bullet
#   BULLET_INCLUDE_DIR     - the Bullet include directory
#   BULLET_LIBRARIES       - all Bullet libraries
#   BULLET_DYNAMICS_LIB    - BulletDynamics
#   BULLET_COLLISION_LIB   - BulletCollision
#   BULLET_LINEARMATH_LIB  - LinearMath

FIND_PACKAGE(Bullet CONFIG QUIET)
IF(TARGET BulletDynamics AND TARGET BulletCollision AND TARGET LinearMath)
    SET(BULLET_FOUND TRUE)
    SET(Bullet_FOUND TRUE)
    SET(BULLET_LIBRARIES BulletDynamics BulletCollision LinearMath)
    IF(NOT TARGET Bullet::Bullet)
        ADD_LIBRARY(Bullet::Bullet INTERFACE IMPORTED)
        SET_TARGET_PROPERTIES(Bullet::Bullet PROPERTIES
            INTERFACE_LINK_LIBRARIES "${BULLET_LIBRARIES}"
        )
        IF(BULLET_INCLUDE_DIRS)
            SET_TARGET_PROPERTIES(Bullet::Bullet PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${BULLET_INCLUDE_DIRS}"
            )
        ENDIF()
    ENDIF()
    RETURN()
ENDIF()

# --- Include directory ---
FIND_PATH(BULLET_INCLUDE_DIR btBulletDynamicsCommon.h
    PATH_SUFFIXES bullet
    PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
    /opt/homebrew/include
)

# --- Library: BulletDynamics ---
FIND_LIBRARY(BULLET_DYNAMICS_LIB BulletDynamics
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /opt/local/lib
    /opt/homebrew/lib
)

FIND_LIBRARY(BULLET_DYNAMICS_DEBUG_LIB BulletDynamics_Debug
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /opt/local/lib
    /opt/homebrew/lib
)

# --- Library: BulletCollision ---
FIND_LIBRARY(BULLET_COLLISION_LIB BulletCollision
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /opt/local/lib
    /opt/homebrew/lib
)

FIND_LIBRARY(BULLET_COLLISION_DEBUG_LIB BulletCollision_Debug
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /opt/local/lib
    /opt/homebrew/lib
)

# --- Library: LinearMath ---
FIND_LIBRARY(BULLET_LINEARMATH_LIB LinearMath
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /opt/local/lib
    /opt/homebrew/lib
)

FIND_LIBRARY(BULLET_LINEARMATH_DEBUG_LIB LinearMath_Debug
    /usr/lib
    /usr/lib64
    /usr/local/lib
    /opt/local/lib
    /opt/homebrew/lib
)

# --- Validate ---
IF(BULLET_INCLUDE_DIR AND BULLET_DYNAMICS_LIB AND BULLET_COLLISION_LIB AND BULLET_LINEARMATH_LIB)
    SET(BULLET_FOUND TRUE)
    SET(Bullet_FOUND TRUE)
    IF(NOT BULLET_DYNAMICS_DEBUG_LIB)
        SET(BULLET_DYNAMICS_DEBUG_LIB ${BULLET_DYNAMICS_LIB})
    ENDIF()
    IF(NOT BULLET_COLLISION_DEBUG_LIB)
        SET(BULLET_COLLISION_DEBUG_LIB ${BULLET_COLLISION_LIB})
    ENDIF()
    IF(NOT BULLET_LINEARMATH_DEBUG_LIB)
        SET(BULLET_LINEARMATH_DEBUG_LIB ${BULLET_LINEARMATH_LIB})
    ENDIF()
    SET(BULLET_LIBRARIES
        $<$<CONFIG:Debug>:${BULLET_DYNAMICS_DEBUG_LIB}>$<$<NOT:$<CONFIG:Debug>>:${BULLET_DYNAMICS_LIB}>
        $<$<CONFIG:Debug>:${BULLET_COLLISION_DEBUG_LIB}>$<$<NOT:$<CONFIG:Debug>>:${BULLET_COLLISION_LIB}>
        $<$<CONFIG:Debug>:${BULLET_LINEARMATH_DEBUG_LIB}>$<$<NOT:$<CONFIG:Debug>>:${BULLET_LINEARMATH_LIB}>
    )
ENDIF()

# --- Messages ---
IF(BULLET_FOUND)
    IF(NOT Bullet_FIND_QUIETLY)
        MESSAGE(STATUS "Found Bullet: ${BULLET_LIBRARIES}")
    ENDIF()
    IF(NOT TARGET Bullet::Bullet)
        ADD_LIBRARY(Bullet::Bullet INTERFACE IMPORTED)
        SET_TARGET_PROPERTIES(Bullet::Bullet PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${BULLET_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${BULLET_LIBRARIES}"
        )
    ENDIF()
ELSE()
    IF(Bullet_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find Bullet Physics")
    ELSE()
        MESSAGE(STATUS "Bullet not found")
    ENDIF()
ENDIF()
