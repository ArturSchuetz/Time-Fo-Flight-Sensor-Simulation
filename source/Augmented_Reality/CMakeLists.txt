
# Check if examples are enabled
if(NOT OPTION_BUILD_AR)
    return()
endif()

# applications
add_subdirectory(01_ArUco_Marker_Detection)