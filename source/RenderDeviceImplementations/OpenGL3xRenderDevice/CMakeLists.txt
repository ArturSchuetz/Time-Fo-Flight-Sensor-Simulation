
# 
# External dependencies
# 

find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIR})
    link_libraries(${OPENGL_LIBRARIES})

    message(STATUS "OPENGL_INCLUDE_DIR: " ${OPENGL_INCLUDE_DIR})
    message(STATUS "OPENGL_LIBRARIES: " ${OPENGL_LIBRARIES})
else()
    message(FATAL_ERROR "${target} skipped: OpenGL not found")
    return()
endif ()

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIR})
    link_libraries(${GLEW_LIBRARY})

    message(STATUS "GLEW_INCLUDE_DIR: " ${GLEW_INCLUDE_DIR})
    message(STATUS "GLEW_LIBRARY: " ${GLEW_LIBRARY})
else()
    message(FATAL_ERROR "${target} skipped: GLEW not found")
    return()
endif ()

find_package(GLFW REQUIRED)
if (GLFW_FOUND)
    include_directories(${GLFW_INCLUDE_DIR})
    link_libraries(${GLFW_LIBRARIES})

    message(STATUS "GLFW_INCLUDE_DIR: " ${GLFW_INCLUDE_DIR})
    message(STATUS "GLFW_LIBRARIES: " ${GLFW_LIBRARIES})
else()
    find_package(glfw3 REQUIRED)
    if (glfw3_FOUND)
        link_libraries(glfw)
        include_directories ($<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>)
    endif()
endif()

# 
# Library name and options
# 

# Target name
set(target OpenGL3xRenderDevice)

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

set(headers
    ${include_path}/Device/GUI/imconfig.h
    ${include_path}/Device/GUI/imgui.h
    ${include_path}/Device/GUI/imgui_internal.h
    ${include_path}/Device/GUI/imstb_rectpack.h
    ${include_path}/Device/GUI/imstb_textedit.h
    ${include_path}/Device/GUI/imstb_truetype.h
    ${include_path}/Device/GUI/imgui_impl_glfw.h
    ${include_path}/Device/GUI/imgui_impl_opengl3.h
    ${include_path}/Device/Buffer/BowOGL3xBuffer.h
    ${include_path}/Device/Buffer/BowOGL3xBufferName.h
    ${include_path}/Device/Buffer/BowOGL3xIndexBuffer.h
    ${include_path}/Device/Buffer/BowOGL3xPixelBuffer.h
    ${include_path}/Device/Buffer/BowOGL3xReadPixelBuffer.h
    ${include_path}/Device/Buffer/BowOGL3xVertexBuffer.h
    ${include_path}/Device/Buffer/BowOGL3xWritePixelBuffer.h
    ${include_path}/Device/Context/FrameBuffer/BowOGL3xColorAttachments.h
    ${include_path}/Device/Context/FrameBuffer/BowOGL3xFramebuffer.h
    ${include_path}/Device/Context/VertexArray/BowOGL3xVertexArray.h
    ${include_path}/Device/Context/VertexArray/BowOGL3xVertexBufferAttributes.h
    ${include_path}/Device/Shader/BowOGL3xFragmentOutputs.h
    ${include_path}/Device/Shader/BowOGL3xShaderObject.h
    ${include_path}/Device/Shader/BowOGL3xShaderProgram.h
    ${include_path}/Device/Shader/BowOGL3xUniform.h
    ${include_path}/Device/Textures/BowOGL3xTexture2D.h
    ${include_path}/Device/Textures/BowOGL3xTextureSampler.h
    ${include_path}/Device/Textures/BowOGL3xTextureUnit.h
    ${include_path}/Device/Textures/BowOGL3xTextureUnits.h
    ${include_path}/Device/BowOGL3xGraphicsWindow.h
    ${include_path}/Device/BowOGL3xRenderContext.h
	${include_path}/BowOGL3xTypeConverter.h
    ${include_path}/BowOGL3xRenderDevice.h
	${include_path}/BowOGL3xRenderDevicePredeclares.h
)

set(sources
	${source_path}/Device/GUI/imgui.cpp
    ${source_path}/Device/GUI/imgui_demo.cpp
    ${source_path}/Device/GUI/imgui_draw.cpp
    ${source_path}/Device/GUI/imgui_widgets.cpp
    ${source_path}/Device/GUI/imgui_impl_glfw.cpp
    ${source_path}/Device/GUI/imgui_impl_opengl3.cpp
	
    ${source_path}/Device/Buffer/BowOGL3xBuffer.cpp
    ${source_path}/Device/Buffer/BowOGL3xBufferName.cpp
    ${source_path}/Device/Buffer/BowOGL3xIndexBuffer.cpp
    ${source_path}/Device/Buffer/BowOGL3xPixelBuffer.cpp
    ${source_path}/Device/Buffer/BowOGL3xReadPixelBuffer.cpp
    ${source_path}/Device/Buffer/BowOGL3xVertexBuffer.cpp
    ${source_path}/Device/Buffer/BowOGL3xWritePixelBuffer.cpp
	
    ${source_path}/Device/Context/FrameBuffer/BowOGL3xColorAttachments.cpp
    ${source_path}/Device/Context/FrameBuffer/BowOGL3xFramebuffer.cpp
	
    ${source_path}/Device/Context/VertexArray/BowOGL3xVertexArray.cpp
    ${source_path}/Device/Context/VertexArray/BowOGL3xVertexBufferAttributes.cpp
	
    ${source_path}/Device/Shader/BowOGL3xFragmentOutputs.cpp
    ${source_path}/Device/Shader/BowOGL3xShaderObject.cpp
    ${source_path}/Device/Shader/BowOGL3xShaderProgram.cpp
    ${source_path}/Device/Shader/BowOGL3xUniformF.cpp
    ${source_path}/Device/Shader/BowOGL3xUniformI.cpp
    ${source_path}/Device/Shader/BowOGL3xUniformUI.cpp
	
    ${source_path}/Device/Textures/BowOGL3xTexture2D.cpp
    ${source_path}/Device/Textures/BowOGL3xTextureSampler.cpp
    ${source_path}/Device/Textures/BowOGL3xTextureUnit.cpp
    ${source_path}/Device/Textures/BowOGL3xTextureUnits.cpp
	
    ${source_path}/Device/BowOGL3xGraphicsWindow.cpp
    ${source_path}/Device/BowOGL3xRendererContext.cpp
    ${source_path}/Device/BowOGL3xRendererContext_GUI.cpp
    ${source_path}/Device/BowOGL3xRendererContext_RenderState.cpp

	${source_path}/BowOGL3xRenderDevice.cpp
	${source_path}/BowOGL3xTypeConverter.cpp
    ${source_path}/DLLExport.cpp
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
add_library(${target} SHARED
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
	${META_PROJECT_NAME}::CoreSystems
	${META_PROJECT_NAME}::RenderDevice
    ${META_PROJECT_NAME}::Resources
	${GLFW_LIBRARIES}
    ${GLEW_LIBRARY}

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
    ${OPTION_GLFW_DEFINE}

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
