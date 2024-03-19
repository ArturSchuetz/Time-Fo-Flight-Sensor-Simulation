
# 
# External dependencies
# 

find_package(CUDA 5.0 REQUIRED)
if(CUDA_FOUND)
    include_directories("${CUDA_INCLUDE_DIRS}")
    link_directories ("${CUDA_LIBRARY_DIRS}")
else()
    message(FATAL_ERROR "CUDA library not found")
    return()
endif()

find_package(OptiX REQUIRED)
include_directories("${OptiX_INCLUDE}")

# 
# Executable name and options
# 

# Target name
set(target 01_RayTracing)

# Exit here if required dependencies are not met
message(STATUS "Application ${target}")


# 
# Sources
# 

set(sources
	Application.h
	Application.cpp
    main.cpp
)


# 
# Create executable
# 

# Build executable
add_executable(${target}
    MACOSX_BUNDLE
    ${source_files}
    ${sources}
    ${generated_files}
    ${cmake_options}
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
    ${META_PROJECT_NAME}::CoreSystems
    ${META_PROJECT_NAME}::Resources
    ${META_PROJECT_NAME}::InputDevice
    ${META_PROJECT_NAME}::RenderDevice
    ${META_PROJECT_NAME}::OptixUtils
    ${META_PROJECT_NAME}::CameraUtils
)

add_custom_command(TARGET ${target} POST_BUILD     # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
		"${optix_DLL}"							   # <--this is in-file
		$<TARGET_FILE_DIR:${target}>)              # <--this is out-file path

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
