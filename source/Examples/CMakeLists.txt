
# Check if examples are enabled
if(NOT OPTION_BUILD_EXAMPLES)
    return()
endif()

# Example applications
add_subdirectory(00_TimerTest)
add_subdirectory(01_Input)
add_subdirectory(02_HelloWorld)
add_subdirectory(03_Triangle)
add_subdirectory(04_MeshRenderer)
add_subdirectory(05_SceneRenderer)