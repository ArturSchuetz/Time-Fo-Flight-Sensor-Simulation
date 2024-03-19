/*
* Copyright (c) 2018 NVIDIA CORPORATION. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once
#include <OptixUtils/OptixUtils_api.h>

#define NOMINMAX 1
#include <optixu/optixpp_namespace.h>

#include <vector>


// Default catch block
#define SUTIL_CATCH( ctx ) catch( sutil::APIError& e ) {           \
    sutil::handleError( ctx, e.code, e.file.c_str(), e.line );     \
  }                                                                \
  catch( std::exception& e ) {                                     \
    sutil::reportErrorMessage( e.what() );                         \
  }

// Error check/report helper for users of the C API
#define RT_CHECK_ERROR( func )                                     \
  do {                                                             \
    RTresult code = func;                                          \
    if( code != RT_SUCCESS )                                       \
      throw sutil::APIError( code, __FILE__, __LINE__ );           \
  } while(0)

namespace sutil
{

	// Exeption to be thrown by RT_CHECK_ERROR macro
	struct APIError
	{
		APIError(RTresult c, const std::string& f, int l) : code(c), file(f), line(l) {}
		RTresult     code;
		std::string  file;
		int          line;
	};

	// Display error message
	void OPTIXUTILS_API reportErrorMessage(
		const char* message);               // Error mssg to be displayed

											// Queries provided RTcontext for last error message, displays it, then exits.
	void OPTIXUTILS_API handleError(
		RTcontext context,                  // Context associated with the error
		RTresult code,                      // Code returned by OptiX API call
		const char* file,                   // Filename for error reporting
		int line);                          // File lineno for error reporting

	void OPTIXUTILS_API checkBuffer(RTbuffer buffer);

	// Create an output buffer with given specifications
	optix::Buffer OPTIXUTILS_API createOutputBuffer(
		optix::Context context,             // optix context
		RTformat format,                    // Pixel format (must be ubyte4 for pbo)
		unsigned width,                     // Buffer width
		unsigned height);                   // Buffer height

	// Create an input/output buffer with given specifications
	optix::Buffer OPTIXUTILS_API createInputOutputBuffer(
		optix::Context context,             // optix context
		RTformat format,                    // Pixel format (must be ubyte4 for pbo)
		unsigned width,                     // Buffer width
		unsigned height);                   // Buffer height  

	// Resize a Buffer and its underlying GLBO if necessary
	void OPTIXUTILS_API resizeBuffer(
		optix::Buffer buffer,               // Buffer to be modified
		unsigned width,                     // New buffer width
		unsigned height);                   // New buffer height

	// Get PTX, either pre-compiled with NVCC or JIT compiled by NVRTC.
	OPTIXUTILS_API const char* getPtxString(
		const char* filename,               // Cuda C input file name
		const char** log = NULL);          // (Optional) pointer to compiler log string. If *log == NULL there is no output. Only valid until the next getPtxString call
										   
	// Create on OptiX TextureSampler for the given image file.  If the filename is
	// empty or if loading the file fails, return 1x1 texture with default color.
	OPTIXUTILS_API optix::TextureSampler loadTexture(
		optix::Context context,			// Context used for object creation 
		const std::string& filename,	// File to load
		optix::float3 default_color);	// Default color in case of file failure

	// Ensures that width and height have the minimum size to prevent launch errors.
	void OPTIXUTILS_API ensureMinimumSize(
		int& width,                             // Will be assigned the minimum suitable width if too small.
		int& height);                           // Will be assigned the minimum suitable height if too small.

	// Ensures that width and height have the minimum size to prevent launch errors.
	void OPTIXUTILS_API ensureMinimumSize(
		unsigned& width,                        // Will be assigned the minimum suitable width if too small.
		unsigned& height);                      // Will be assigned the minimum suitable height if too small.
}