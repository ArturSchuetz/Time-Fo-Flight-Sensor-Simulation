
# 
# External dependencies
# 

# find_package(THIRDPARTY REQUIRED)


# 
# Library name and options
# 

# Target name
set(target Resources)

# Exit here if required dependencies are not met
message(STATUS "Lib ${target}")

# Set API export file and macro
string(MAKE_C_IDENTIFIER ${target} target_id)
string(TOUPPER ${target_id} target_id)
set(feature_file         "include/${target}/${target}_features.h")
set(export_file          "include/${target}/${target}_export.h")
set(template_export_file "include/${target}/${target}_api.h")
set(export_macro         "${target_id}_API")


# 
# Sources
# 

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/include/${target}")
set(source_path  "${CMAKE_CURRENT_SOURCE_DIR}/source")

# Root Folder
set(headers
    ${include_path}/FileLoader/ImageLoader/BowImageLoader_bmp.h
    ${include_path}/FileLoader/ImageLoader/BowImageLoader_hdr.h
    ${include_path}/FileLoader/ImageLoader/BowImageLoader_png.h
    ${include_path}/FileLoader/ImageLoader/BowImageLoader_tga.h
    ${include_path}/FileLoader/MeshLoader/BowModelLoader_obj.h
    ${include_path}/FileLoader/MeshLoader/BowModelLoader_ply.h
    ${include_path}/FileLoader/PointCloudLoader/BowPointCloudLoader_bin.h
    ${include_path}/FileLoader/PointCloudLoader/BowPointCloudLoader_xcn.h
    ${include_path}/FileLoader/PointCloudLoader/BowPointCloudLoader_xtc.h
    ${include_path}/FileLoader/PointCloudLoader/BowPointCloudLoader_xyz.h
    ${include_path}/ResourceManagers/BowImageManager.h
    ${include_path}/ResourceManagers/BowMaterialManager.h
    ${include_path}/ResourceManagers/BowMeshManager.h
    ${include_path}/ResourceManagers/BowPointCloudManager.h
    ${include_path}/Resources/BowImage.h
    ${include_path}/Resources/BowMaterial.h
    ${include_path}/Resources/BowMesh.h
    ${include_path}/Resources/BowPointCloud.h
    ${include_path}/BowResources.h
    ${include_path}/BowResourcesPredeclares.h
    ${include_path}/BowResource.h
    ${include_path}/BowResourceManager.h
)

set(sources
    ${source_path}/FileLoader/ImageLoader/BowImageLoader_bmp.cpp
    ${source_path}/FileLoader/ImageLoader/BowImageLoader_hdr.cpp
    ${source_path}/FileLoader/ImageLoader/BowImageLoader_png.cpp
    ${source_path}/FileLoader/ImageLoader/BowImageLoader_tga.cpp
    ${source_path}/FileLoader/MeshLoader/BowModelLoader_obj.cpp
    ${source_path}/FileLoader/MeshLoader/BowModelLoader_ply.cpp
    ${source_path}/FileLoader/PointCloudLoader/BowPointCloudLoader_bin.cpp
    ${source_path}/FileLoader/PointCloudLoader/BowPointCloudLoader_xcn.cpp
    ${source_path}/FileLoader/PointCloudLoader/BowPointCloudLoader_xtc.cpp
    ${source_path}/FileLoader/PointCloudLoader/BowPointCloudLoader_xyz.cpp
    ${source_path}/ResourceManagers/BowImageManager.cpp
    ${source_path}/ResourceManagers/BowMaterialManager.cpp
    ${source_path}/ResourceManagers/BowMeshManager.cpp
    ${source_path}/ResourceManagers/BowPointCloudManager.cpp
    ${source_path}/Resources/BowImage.cpp
    ${source_path}/Resources/BowMaterial.cpp
    ${source_path}/Resources/BowMesh.cpp
    ${source_path}/Resources/BowPointCloud.cpp
    ${source_path}/BowResource.cpp
    ${source_path}/BowResourceManager.cpp
)

# Group source files
set(header_group "Header Files (API)")
set(source_group "Source Files")
source_group_by_path(${include_path} "\\\\.h$|\\\\.hpp$" 
    ${header_group} ${headers})
source_group_by_path(${source_path}  "\\\\.cpp$|\\\\.c$|\\\\.h$|\\\\.hpp$" 
    ${source_group} ${sources})

# 
# Create library
# 

# Build library
add_library(${target} STATIC
    ${sources}
    ${headers}
)

# Create namespaced alias
add_library(${META_PROJECT_NAME}::${target} ALIAS ${target})

# Export library for downstream projects
export(TARGETS ${target} NAMESPACE ${META_PROJECT_NAME}:: FILE ${PROJECT_BINARY_DIR}/cmake/${target}/${target}-export.cmake)

# Create feature detection header
# Compilers: https://cmake.org/cmake/help/v3.1/variable/CMAKE_LANG_COMPILER_ID.html#variable:CMAKE_%3CLANG%3E_COMPILER_ID
# Feature: https://cmake.org/cmake/help/v3.1/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html

# Check for availability of module; use pre-generated version if not found
if (WriterCompilerDetectionHeaderFound)
    write_compiler_detection_header(
        FILE ${feature_file}
        PREFIX ${target_id}
        COMPILERS AppleClang Clang GNU MSVC
        FEATURES cxx_alignas cxx_alignof cxx_constexpr cxx_final cxx_noexcept cxx_nullptr cxx_sizeof_member cxx_thread_local
        VERSION 3.2
    )
else()
    file(
        COPY ${PROJECT_SOURCE_DIR}/source/codegeneration/${target}_features.h
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/${target}
        USE_SOURCE_PERMISSIONS
    )
endif()

# Create API export header
generate_export_header(${target}
    EXPORT_FILE_NAME  ${export_file}
    EXPORT_MACRO_NAME ${export_macro}
)
generate_template_export_header(${target}
    ${target_id}
    ${template_export_file}
)


# 
# Project options
# 

set_target_properties(${target}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
    FOLDER "${IDE_FOLDER}"
    VERSION ${META_VERSION}
    SOVERSION ${META_VERSION_MAJOR}
)


# 
# Include directories
# 

target_include_directories(${target}
    PRIVATE
    ${PROJECT_BINARY_DIR}/source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include

    PUBLIC
    ${DEFAULT_INCLUDE_DIRECTORIES}

    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)


# 
# Libraries
# 

target_link_libraries(${target}
    PRIVATE
	${META_PROJECT_NAME}::LoadPNG
    ${META_PROJECT_NAME}::Platform
    ${META_PROJECT_NAME}::CoreSystems

    PUBLIC
    ${DEFAULT_LIBRARIES}

    INTERFACE
)


# 
# Compile definitions
# 

target_compile_definitions(${target}
    PRIVATE

    PUBLIC
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:${target_id}_STATIC_DEFINE>
    ${DEFAULT_COMPILE_DEFINITIONS}

    INTERFACE
)


# 
# Compile options
# 

target_compile_options(${target}
    PRIVATE

    PUBLIC
    ${DEFAULT_COMPILE_OPTIONS}

    INTERFACE
)


# 
# Linker options
# 

target_link_libraries(${target}
    PRIVATE

    PUBLIC
    ${DEFAULT_LINKER_OPTIONS}

    INTERFACE
)


#
# Target Health
#

perform_health_checks(
    ${target}
    ${sources}
    ${headers}
)


# 
# Deployment
# 

# Library
install(TARGETS ${target}
    EXPORT  "${target}-export"            COMPONENT dev
    RUNTIME DESTINATION ${INSTALL_BIN}    COMPONENT runtime
    LIBRARY DESTINATION ${INSTALL_SHARED} COMPONENT runtime
    ARCHIVE DESTINATION ${INSTALL_LIB}    COMPONENT dev
)

# Header files
install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/include/${target} DESTINATION ${INSTALL_INCLUDE}
    COMPONENT dev
)

# Generated header files
install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/include/${target} DESTINATION ${INSTALL_INCLUDE}
    COMPONENT dev
)

# CMake config
install(EXPORT ${target}-export
    NAMESPACE   ${META_PROJECT_NAME}::
    DESTINATION ${INSTALL_CMAKE}/${target}
    COMPONENT   dev
)
