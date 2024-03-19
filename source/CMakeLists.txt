
# 
# Configuration for all sub-projects
# 

# Generate version-header
configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/${META_PROJECT_NAME}/${META_PROJECT_NAME}-version.h)

# 
# Sub-projects
# 

# Engine
set(IDE_FOLDER "Engine")
add_subdirectory(InputDevice)
add_subdirectory(InputDeviceImplementations)
add_subdirectory(RenderDevice)
add_subdirectory(RenderDeviceImplementations)
add_subdirectory(CoreSystems)
add_subdirectory(Resources)
add_subdirectory(Platform)
add_subdirectory(ThirdParty)

# Examples
set(IDE_FOLDER "Examples")
add_subdirectory(Examples)

set(IDE_FOLDER "Augmented_Reality")
add_subdirectory(Augmented_Reality)

# Libraries
set(IDE_FOLDER "Libraries")
add_subdirectory(Libraries)

# Applications
set(IDE_FOLDER "TOF_Simulation")
add_subdirectory(TOF_Simulation)

set(IDE_FOLDER "TOF_Evaluation")
add_subdirectory(TOF_Evaluation)

# TestCases
if(OPTION_BUILD_TESTS AND NOT MINGW)
    set(IDE_FOLDER "TestCases")
    add_subdirectory(Tests)
endif()

# 
# Deployment
# 

# Deploy generated headers
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/${META_PROJECT_NAME} DESTINATION include COMPONENT dev)
