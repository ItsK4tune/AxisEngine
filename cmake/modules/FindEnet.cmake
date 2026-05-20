# - Try to find ENet
# Once done, this will define
#
# ENET_FOUND - system has ENet
# ENET_INCLUDE_DIR - the ENet include directories
# ENET_LIBRARIES - link these to use ENet

FIND_PATH( ENET_INCLUDE_DIR enet/enet.h
	/usr/include
	/usr/local/include
	/opt/local/include
	${CMAKE_SOURCE_DIR}/include
)

FIND_LIBRARY( ENET_LIBRARY enet
	/usr/lib64
	/usr/lib
	/usr/local/lib
	/opt/local/lib
	${CMAKE_SOURCE_DIR}/lib
)

IF(ENET_INCLUDE_DIR AND ENET_LIBRARY)
	SET( ENET_FOUND TRUE )
	SET( ENET_LIBRARIES ${ENET_LIBRARY} )
ENDIF(ENET_INCLUDE_DIR AND ENET_LIBRARY)

IF(ENET_FOUND)
	IF(NOT ENET_FIND_QUIETLY)
		MESSAGE(STATUS "Found ENET: ${ENET_LIBRARY}")
	ENDIF(NOT ENET_FIND_QUIETLY)
ELSE(ENET_FOUND)
	IF(ENET_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could not find ENet")
	ENDIF(ENET_FIND_REQUIRED)
ENDIF(ENET_FOUND)
