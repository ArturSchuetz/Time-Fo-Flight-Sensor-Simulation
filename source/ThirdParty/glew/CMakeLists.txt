CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )
PROJECT( "glew" )

SET( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src )
SET( INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include )
SET( RC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build )

OPTION( glew-cmake_BUILD_SHARED "Build the shared glew library" ON )
OPTION( glew-cmake_BUILD_STATIC "Build the static glew library" ON )
OPTION( glew-cmake_BUILD_SINGLE_CONTEXT "Build the single context glew library" ON )
OPTION( glew-cmake_BUILD_MULTI_CONTEXT "Build the multi context glew library" ON )


SET( LIBGLEW_SRCS ${SRC_DIR}/glew.c )

IF( WIN32 )
	ADD_DEFINITIONS( -DWIN32_MEAN_AND_LEAN -DVC_EXTRALEAN -DGLEW_BUILD -D_CRT_SECURE_NO_WARNINGS )
ENDIF(  )

IF( MSVC )
	LIST( APPEND LIBGLEW_SRCS ${RC_DIR}/glew.rc )
ENDIF(  )

IF( NOT USE_GLU )
	ADD_DEFINITIONS( -DGLEW_NO_GLU )
ENDIF(  )

IF( WIN32 )
	SET( GL_LIBRARY opengl32 )
ELSEIF( APPLE )
	FIND_LIBRARY( GL_LIBRARY OpenGL )
	FIND_LIBRARY( AGL_LIBRARY AGL )
	SET( PLATFORM_SPECIFIC_LIBRARIES ${AGL_LIBRARY} )
ELSE(  )
	SET( GL_LIBRARY GL )
	SET( PLATFORM_SPECIFIC_LIBRARIES X11 dl Xext )
ENDIF(  )

IF ( glew-cmake_BUILD_STATIC )
	IF ( glew-cmake_BUILD_SINGLE_CONTEXT )
		ADD_LIBRARY( libglew_static STATIC ${LIBGLEW_SRCS} )
		
		export(TARGETS libglew_static NAMESPACE ${META_PROJECT_NAME}:: FILE ${PROJECT_BINARY_DIR}/cmake/libglew_static/libglew_static-export.cmake)
		
		SET_TARGET_PROPERTIES( libglew_static PROPERTIES
		                       ARCHIVE_OUTPUT_NAME "glew"
		                       DEBUG_POSTFIX d 
							   FOLDER "ThirdParty" )
		TARGET_INCLUDE_DIRECTORIES( libglew_static PUBLIC ${INCLUDE_DIR} )
		TARGET_LINK_LIBRARIES( libglew_static ${GL_LIBRARY} ${PLATFORM_SPECIFIC_LIBRARIES} )

		install(TARGETS libglew_static
			RUNTIME DESTINATION ${INSTALL_BIN}    COMPONENT runtime
			LIBRARY DESTINATION ${INSTALL_SHARED} COMPONENT runtime
			ARCHIVE DESTINATION ${INSTALL_LIB}    COMPONENT dev
		)
		
		# # Header files
		# install(DIRECTORY
			# ${CMAKE_CURRENT_SOURCE_DIR}/include/libglew_static DESTINATION ${INSTALL_INCLUDE}
			# COMPONENT dev
		# )

		# # Generated header files
		# install(DIRECTORY
			# ${CMAKE_CURRENT_BINARY_DIR}/include/libglew_static DESTINATION ${INSTALL_INCLUDE}
			# COMPONENT dev
		# )
		
		# install(EXPORT libglew_static-export
			# NAMESPACE   ${META_PROJECT_NAME}::
			# DESTINATION ${INSTALL_CMAKE}/libglew_static
			# COMPONENT   dev
		# )
	ENDIF (  )

	IF ( glew-cmake_BUILD_MULTI_CONTEXT )
		ADD_LIBRARY( libglewmx_static STATIC ${LIBGLEW_SRCS} )

		export(TARGETS libglewmx_static NAMESPACE ${META_PROJECT_NAME}:: FILE ${PROJECT_BINARY_DIR}/cmake/libglewmx_static/libglewmx_static-export.cmake)
		
		SET_TARGET_PROPERTIES( libglewmx_static PROPERTIES
		                       ARCHIVE_OUTPUT_NAME "glewmx"
		                       DEBUG_POSTFIX d 
							   FOLDER "ThirdParty" )
		TARGET_COMPILE_DEFINITIONS( libglewmx_static INTERFACE GLEW_MX)
		TARGET_INCLUDE_DIRECTORIES( libglewmx_static PUBLIC ${INCLUDE_DIR} )
		TARGET_LINK_LIBRARIES( libglewmx_static ${GL_LIBRARY} ${PLATFORM_SPECIFIC_LIBRARIES} )

		install(TARGETS libglewmx_static
			RUNTIME DESTINATION ${INSTALL_BIN}    COMPONENT runtime
			LIBRARY DESTINATION ${INSTALL_SHARED} COMPONENT runtime
			ARCHIVE DESTINATION ${INSTALL_LIB}    COMPONENT dev
		)
		
		# # Header files
		# install(DIRECTORY
			# ${CMAKE_CURRENT_SOURCE_DIR}/include/libglewmx_static DESTINATION ${INSTALL_INCLUDE}
			# COMPONENT dev
		# )

		# # Generated header files
		# install(DIRECTORY
			# ${CMAKE_CURRENT_BINARY_DIR}/include/libglewmx_static DESTINATION ${INSTALL_INCLUDE}
			# COMPONENT dev
		# )
		
		# install(EXPORT libglewmx_static-export
			# NAMESPACE   ${META_PROJECT_NAME}::
			# DESTINATION ${INSTALL_CMAKE}/libglewmx_static
			# COMPONENT   dev
		# )
	ENDIF (  )
ENDIF (  )

IF ( glew-cmake_BUILD_SHARED )
	IF ( glew-cmake_BUILD_SINGLE_CONTEXT )
		ADD_LIBRARY( libglew_shared SHARED ${LIBGLEW_SRCS} )
		
		export(TARGETS libglew_shared NAMESPACE ${META_PROJECT_NAME}:: FILE ${PROJECT_BINARY_DIR}/cmake/libglew_shared/libglew_shared-export.cmake)
		
		SET_TARGET_PROPERTIES( libglew_shared PROPERTIES
		                       RUNTIME_OUTPUT_NAME "glew"
		                       LIBRARY_OUTPUT_NAME "glew"
		                       DEBUG_POSTFIX d 
							   FOLDER "ThirdParty" )

		TARGET_INCLUDE_DIRECTORIES( libglew_shared PUBLIC ${INCLUDE_DIR} )
		TARGET_LINK_LIBRARIES( libglew_shared ${GL_LIBRARY} ${PLATFORM_SPECIFIC_LIBRARIES} )

		install(TARGETS libglew_shared
			RUNTIME DESTINATION ${INSTALL_BIN}    COMPONENT runtime
			LIBRARY DESTINATION ${INSTALL_SHARED} COMPONENT runtime
			ARCHIVE DESTINATION ${INSTALL_LIB}    COMPONENT dev
		)
		
		# # Header files
		# install(DIRECTORY
			# ${CMAKE_CURRENT_SOURCE_DIR}/include/libglew_shared DESTINATION ${INSTALL_INCLUDE}
			# COMPONENT dev
		# )

		# # Generated header files
		# install(DIRECTORY
			# ${CMAKE_CURRENT_BINARY_DIR}/include/libglew_shared DESTINATION ${INSTALL_INCLUDE}
			# COMPONENT dev
		# )
		
		# install(EXPORT libglew_shared-export
			# NAMESPACE   ${META_PROJECT_NAME}::
			# DESTINATION ${INSTALL_CMAKE}/libglew_shared
			# COMPONENT   dev
		# )
	ENDIF (  )

	IF ( glew-cmake_BUILD_MULTI_CONTEXT )
		ADD_LIBRARY( libglewmx_shared SHARED ${LIBGLEW_SRCS} )

		export(TARGETS libglewmx_shared NAMESPACE ${META_PROJECT_NAME}:: FILE ${PROJECT_BINARY_DIR}/cmake/libglewmx_shared/libglewmx_shared-export.cmake)
		
		SET_TARGET_PROPERTIES( libglewmx_shared PROPERTIES
		                       RUNTIME_OUTPUT_NAME "glewmx"
		                       LIBRARY_OUTPUT_NAME "glewmx"
		                       DEBUG_POSTFIX d 
							   FOLDER "ThirdParty" )

		TARGET_COMPILE_DEFINITIONS( libglewmx_shared INTERFACE GLEW_MX)
		TARGET_INCLUDE_DIRECTORIES( libglewmx_shared PUBLIC ${INCLUDE_DIR} )
		TARGET_LINK_LIBRARIES( libglewmx_shared ${GL_LIBRARY} ${PLATFORM_SPECIFIC_LIBRARIES} )
		
		install(TARGETS libglewmx_shared
			RUNTIME DESTINATION ${INSTALL_BIN}    COMPONENT runtime
			LIBRARY DESTINATION ${INSTALL_SHARED} COMPONENT runtime
			ARCHIVE DESTINATION ${INSTALL_LIB}    COMPONENT dev
		)
		
		# # Header files
		# install(DIRECTORY
			# ${CMAKE_CURRENT_SOURCE_DIR}/include/libglewmx_shared DESTINATION ${INSTALL_INCLUDE}
			# COMPONENT dev
		# )

		# # Generated header files
		# install(DIRECTORY
			# ${CMAKE_CURRENT_BINARY_DIR}/include/libglewmx_shared DESTINATION ${INSTALL_INCLUDE}
			# COMPONENT dev
		# )
		
		# # CMake config
		# install(EXPORT libglewmx_shared-export
			# NAMESPACE   ${META_PROJECT_NAME}::
			# DESTINATION ${INSTALL_CMAKE}/libglewmx_shared
			# COMPONENT   dev
		# )
	ENDIF (  )
ENDIF (  )

FILE( GLOB PUBLIC_HEADERS "include/GL/*.h" )
#INSTALL( FILES ${PUBLIC_HEADERS} DESTINATION include/GL/ )

IF( ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR} AND NOT ONLY_LIBS )
	SET( GLEWINFO_SRCS ${SRC_DIR}/glewinfo.c )
	SET( VISUALINFO_SRCS ${SRC_DIR}/visualinfo.c )
	IF( MSVS )
		LIST( APPEND GLEWINFO_SRCS ${RC_DIR}/glewinfo.rc )
		LIST( APPEND VISUALINFO_SRCS ${RC_DIR}/visualinfo.rc )
	ENDIF(  )

	ADD_EXECUTABLE( glewinfo ${GLEWINFO_SRCS} )
	ADD_EXECUTABLE( visualinfo ${VISUALINFO_SRCS} )

	IF ( glew-cmake_BUILD_STATIC )
		TARGET_LINK_LIBRARIES( glewinfo libglew_static )
		TARGET_LINK_LIBRARIES( visualinfo libglew_static )
	ELSE ( )
		TARGET_LINK_LIBRARIES( glewinfo libglew_shared )
		TARGET_LINK_LIBRARIES( visualinfo libglew_shared )
	ENDIF (  )

	install(TARGETS glewinfo visualinfo
		RUNTIME DESTINATION ${INSTALL_BIN}    COMPONENT runtime
		LIBRARY DESTINATION ${INSTALL_SHARED} COMPONENT runtime
		ARCHIVE DESTINATION ${INSTALL_LIB}    COMPONENT dev
	)
ENDIF(  )
