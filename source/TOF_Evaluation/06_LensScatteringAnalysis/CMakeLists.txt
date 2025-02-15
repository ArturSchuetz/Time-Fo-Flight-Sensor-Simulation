
# 
# External dependencies
# 


find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
    include_directories("${OpenCV_INCLUDE_DIRS}")
    link_directories ("${OpenCV_LIBRARY_DIRS}")
else()
    message(FATAL_ERROR "CUDA library not found")
    return()
endif()

# 
# Executable name and options
# 

# Target name
set(target 06_LensScatteringAnalysis)

# Exit here if required dependencies are not met
message(STATUS "TOF_Evaluation ${target}")


# 
# Sources
# 

set(sources
    main.cpp
)


# 
# Create executable
# 

# Build executable
add_executable(${target}
    MACOSX_BUNDLE
    ${sources}
)

# Create namespaced alias
add_executable(${META_PROJECT_NAME}::${target} ALIAS ${target})


# 
# Project options
# 

set_target_properties(${target}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
    FOLDER "${IDE_FOLDER}"
)


# 
# Include directories
# 

target_include_directories(${target}
    PRIVATE
    ${DEFAULT_INCLUDE_DIRECTORIES}
    ${PROJECT_BINARY_DIR}/source/include
)


# 
# Libraries
# 

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LIBRARIES}
	${OpenCV_LIBS}
    ${META_PROJECT_NAME}::CoreSystems
    ${META_PROJECT_NAME}::Resources
    ${META_PROJECT_NAME}::InputDevice
    ${META_PROJECT_NAME}::RenderDevice
	${META_PROJECT_NAME}::EvaluationUtils
	${META_PROJECT_NAME}::CameraUtils
)

# 
# Compile definitions
# 

target_compile_definitions(${target}
    PRIVATE
    ${DEFAULT_COMPILE_DEFINITIONS}
)


# 
# Compile options
# 

target_compile_options(${target}
    PRIVATE
    ${DEFAULT_COMPILE_OPTIONS}
)


# 
# Linker options
# 

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LINKER_OPTIONS}
)


#
# Target Health
#

perform_health_checks(
    ${target}
    ${sources}
)


# 
# Deployment
# 

# Executable
install(TARGETS ${target}
    RUNTIME DESTINATION ${INSTALL_BIN} COMPONENT examples
    BUNDLE  DESTINATION ${INSTALL_BIN} COMPONENT examples
)
