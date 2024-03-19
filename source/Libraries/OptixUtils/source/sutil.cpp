#include <OptixUtils/sutil.h>
#include <Masterthesis/cuda_config.h>

#include <Resources/Resources/BowMesh.h>
#include <Resources/Resources/BowMaterial.h>
#include <Resources/Resources/BowImage.h>

#include <Resources/ResourceManagers/BowMeshManager.h>
#include <Resources/ResourceManagers/BowMaterialManager.h>
#include <Resources/ResourceManagers/BowImageManager.h>

#include <optixu/optixu_math_namespace.h>

#include <nvrtc.h>

#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <sstream>
#include <map>
#include <memory>

#if defined(_WIN32)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN 1
#    endif
#    include<windows.h>
#    include<mmsystem.h>
#else // Apple and Linux both use this
#    include<sys/time.h>
#    include <unistd.h>
#    include <dirent.h>
#endif

namespace sutil
{
	void reportErrorMessage(const char* message)
	{
		std::cerr << "OptiX Error: '" << message << "'\n";
	#if defined(_WIN32) && defined(RELEASE_PUBLIC)
		{
			char s[2048];
			sprintf(s, "OptiX Error: %s", message);
			MessageBox(0, s, "OptiX Error", MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
		}
	#endif
	}

	void handleError(RTcontext context, RTresult code, const char* file, int line)
	{
		const char* message;
		char s[2048];
		rtContextGetErrorString(context, code, &message);
		sprintf(s, "%s\n(%s:%d)", message, file, line);
		reportErrorMessage(s);
	}

	#define STRINGIFY(x) STRINGIFY2(x)
	#define STRINGIFY2(x) #x
	#define LINE_STR STRINGIFY(__LINE__)

	// Error check/report helper for users of the C API
	#define NVRTC_CHECK_ERROR( func )                                  \
		do {                                                             \
		nvrtcResult code = func;                                       \
		if( code != NVRTC_SUCCESS )                                    \
			throw optix::Exception( "ERROR: " __FILE__ "(" LINE_STR "): " +     \
				std::string( nvrtcGetErrorString( code ) ) );            \
		} while( 0 )

	static bool readSourceFile(std::string &str, const std::string &filename)
	{
		// Try to open file
		std::ifstream file(filename.c_str());
		if (file.good())
		{
			// Found usable source file
			std::stringstream source_buffer;
			source_buffer << file.rdbuf();
			str = source_buffer.str();
			return true;
		}
		return false;
	}

	const char* getBaseDir()
	{
		static char s[512];

		// Allow for overrides.
		const char* dir = PROJECT_BASE_DIR;
		if (dir) {
			strcpy(s, dir);
			return s;
		}

		// Last resort.
		return ".";
	}

	void checkBuffer(RTbuffer buffer)
	{
		// Check to see if the buffer is two dimensional
		unsigned int dimensionality;
		RT_CHECK_ERROR(rtBufferGetDimensionality(buffer, &dimensionality));
		if (2 != dimensionality)
			throw optix::Exception("Attempting to display non-2D buffer");

		// Check to see if the buffer is of type float{1,3,4} or uchar4
		RTformat format;
		RT_CHECK_ERROR(rtBufferGetFormat(buffer, &format));
		if (RT_FORMAT_FLOAT != format &&
			RT_FORMAT_FLOAT4 != format &&
			RT_FORMAT_FLOAT3 != format &&
			RT_FORMAT_UNSIGNED_BYTE3 != format &&
			RT_FORMAT_UNSIGNED_BYTE4 != format)
			throw optix::Exception("Attempting to display buffer with format not float, float3, float4, uchar3 or uchar4");
	}

	optix::Buffer createBufferImpl(
		optix::Context context,
		RTformat format,
		unsigned width,
		unsigned height,
		RTbuffertype buffer_type)
	{
		optix::Buffer buffer;
		buffer = context->createBuffer(buffer_type, format, width, height);

		return buffer;
	}

	optix::Buffer createOutputBuffer(
		optix::Context context,
		RTformat format,
		unsigned width,
		unsigned height)
	{
		return createBufferImpl(context, format, width, height, RT_BUFFER_OUTPUT);
	}

	optix::Buffer createInputOutputBuffer(optix::Context context,
		RTformat format,
		unsigned width,
		unsigned height)
	{
		return createBufferImpl(context, format, width, height, RT_BUFFER_INPUT_OUTPUT);
	}

	void resizeBuffer(optix::Buffer buffer, unsigned width, unsigned height)
	{
		buffer->setSize(width, height);
	}

#if CUDA_NVRTC_ENABLED

	static void getCuStringFromFile(std::string &cu, std::string& location, const char* filename)
	{
		std::vector<std::string> source_locations;

		std::string base_dir = std::string( getBaseDir() );

		// Potential source locations (in priority order)
		source_locations.push_back(base_dir + "/cuda/" + filename);

		for (std::vector<std::string>::const_iterator it = source_locations.begin(); it != source_locations.end(); ++it) {
			// Try to get source code from file
			if (readSourceFile(cu, *it))
			{
				location = *it;
				return;
			}

		}

		// Wasn't able to find or open the requested file
		throw optix::Exception("Couldn't open source file " + std::string(filename));
	}

	static std::string g_nvrtcLog;

	static void getPtxFromCuString(std::string &ptx, const char* cu_source, const char* name, const char** log_string)
	{
		// Create program
		nvrtcProgram prog = 0;
		NVRTC_CHECK_ERROR(nvrtcCreateProgram(&prog, cu_source, name, 0, NULL, NULL));

		// Gather NVRTC options
		std::vector<const char *> options;

		std::string base_dir = std::string(getBaseDir());

		// Collect include dirs
		std::vector<std::string> include_dirs;
		const char *abs_dirs[] = { ABSOLUTE_INCLUDE_DIRS };
		const char *rel_dirs[] = { RELATIVE_INCLUDE_DIRS };

		const size_t n_abs_dirs = sizeof(abs_dirs) / sizeof(abs_dirs[0]);
		for (size_t i = 0; i < n_abs_dirs; i++)
			include_dirs.push_back(std::string("-I") + abs_dirs[i]);
		const size_t n_rel_dirs = sizeof(rel_dirs) / sizeof(rel_dirs[0]);
		for (size_t i = 0; i < n_rel_dirs; i++)
			include_dirs.push_back(std::string("-I") + base_dir + rel_dirs[i]);
		for (std::vector<std::string>::const_iterator it = include_dirs.begin(); it != include_dirs.end(); ++it)
			options.push_back(it->c_str());

		// Collect NVRTC options
		const char *compiler_options[] = { CUDA_NVRTC_OPTIONS };
		const size_t n_compiler_options = sizeof(compiler_options) / sizeof(compiler_options[0]);
		for (size_t i = 0; i < n_compiler_options - 1; i++)
			options.push_back(compiler_options[i]);

		// JIT compile CU to PTX
		const nvrtcResult compileRes = nvrtcCompileProgram(prog, (int)options.size(), options.data());

		// Retrieve log output
		size_t log_size = 0;
		NVRTC_CHECK_ERROR(nvrtcGetProgramLogSize(prog, &log_size));
		g_nvrtcLog.resize(log_size);
		if (log_size > 1)
		{
			NVRTC_CHECK_ERROR(nvrtcGetProgramLog(prog, &g_nvrtcLog[0]));
			if (log_string)
				*log_string = g_nvrtcLog.c_str();
		}
		if (compileRes != NVRTC_SUCCESS)
			throw optix::Exception("NVRTC Compilation failed.\n" + g_nvrtcLog);

		// Retrieve PTX code
		size_t ptx_size = 0;
		NVRTC_CHECK_ERROR(nvrtcGetPTXSize(prog, &ptx_size));
		ptx.resize(ptx_size);
		NVRTC_CHECK_ERROR(nvrtcGetPTX(prog, &ptx[0]));

		// Cleanup
		NVRTC_CHECK_ERROR(nvrtcDestroyProgram(&prog));
	}

#else // CUDA_NVRTC_ENABLED

	static void getPtxStringFromFile(std::string &ptx, const char* filename)
	{
		std::string source_filename = "cuda_compile_ptx_generated_" + std::string(filename) + ".ptx";

		// Try to open source PTX file
		if (!readSourceFile(ptx, source_filename))
			throw optix::Exception("Couldn't open source file " + source_filename);
	}

#endif // CUDA_NVRTC_ENABLED

	struct PtxSourceCache
	{
		std::map<std::string, std::string *> map;
		~PtxSourceCache()
		{
			for (std::map<std::string, std::string *>::const_iterator it = map.begin(); it != map.end(); ++it)
				delete it->second;
		}
	};

	static PtxSourceCache g_ptxSourceCache;

	const char* sutil::getPtxString(const char* filename, const char** log)
	{
		if (log)
			*log = NULL;

		std::string *ptx, cu;
		std::string key = std::string(filename);
		std::map<std::string, std::string *>::iterator elem = g_ptxSourceCache.map.find(key);

		if (elem == g_ptxSourceCache.map.end())
		{
			ptx = new std::string();
#if CUDA_NVRTC_ENABLED
			std::string location;
			getCuStringFromFile(cu, location, filename);
			getPtxFromCuString(*ptx, cu.c_str(), location.c_str(), log);
#else
			getPtxStringFromFile(*ptx, filename);
#endif
			g_ptxSourceCache.map[key] = ptx;
		}
		else
		{
			ptx = elem->second;
		}

		return ptx->c_str();
	}

	void ensureMinimumSize(int& w, int& h)
	{
		if (w <= 0) w = 1;
		if (h <= 0) h = 1;
	}

	void ensureMinimumSize(unsigned& w, unsigned& h)
	{
		if (w == 0) w = 1;
		if (h == 0) h = 1;
	}

	optix::TextureSampler sutil::loadTexture(optix::Context context, const std::string& filename, optix::float3 default_color)
	{
		bow::ImagePtr image = bow::ImageManager::GetInstance().Load(filename);

		// Create tex sampler and populate with default values
 		optix::TextureSampler sampler = context->createTextureSampler();
		sampler->setWrapMode(0, RT_WRAP_REPEAT);
		sampler->setWrapMode(1, RT_WRAP_REPEAT);
		sampler->setWrapMode(2, RT_WRAP_REPEAT);
		sampler->setIndexingMode(RT_TEXTURE_INDEX_NORMALIZED_COORDINATES);
		sampler->setReadMode(RT_TEXTURE_READ_ELEMENT_TYPE);
		sampler->setMaxAnisotropy(1.0f);
		sampler->setMipLevelCount(1u);
		sampler->setArraySize(1u);

		if (image->VGetSizeInBytes() == 0) {

			// Create buffer with single texel set to default_color
			optix::Buffer buffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT4, 1u, 1u);
			float* buffer_data = static_cast<float*>(buffer->map());
			buffer_data[0] = default_color.x;
			buffer_data[1] = default_color.y;
			buffer_data[2] = default_color.z;
			buffer_data[3] = 1.0f;
			buffer->unmap();

			sampler->setBuffer(0u, 0u, buffer);
			// Although it would be possible to use nearest filtering here, we chose linear
			// to be consistent with the textures that have been loaded from a file. This
			// allows OptiX to perform some optimizations.
			sampler->setFilteringModes(RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_FILTER_NONE);

			return sampler;
		}

		const unsigned int nx = image->GetWidth();
		const unsigned int ny = image->GetHeight();

		// Create buffer and populate with PPM data
		optix::Buffer buffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT4, nx, ny);
		float* buffer_data = static_cast<float*>(buffer->map());

		if (image->GetNumChannels() == 4)
		{
			for (unsigned int i = 0; i < nx; ++i)
			{
				for (unsigned int j = 0; j < ny; ++j)
				{
					unsigned int ppm_index = ((ny - j - 1) * nx + i) * 4;
					unsigned int buf_index = ((j)* nx + i) * 4;

					buffer_data[buf_index + 0] = image->GetData()[ppm_index + 0];
					buffer_data[buf_index + 1] = image->GetData()[ppm_index + 1];
					buffer_data[buf_index + 2] = image->GetData()[ppm_index + 2];
					buffer_data[buf_index + 3] = image->GetData()[ppm_index + 3];
				}
			}
		}
		else if (image->GetNumChannels() == 3)
		{
			for (unsigned int i = 0; i < nx; ++i)
			{
				for (unsigned int j = 0; j < ny; ++j)
				{
					unsigned int ppm_index = ((ny - j - 1) * nx + i) * 3;
					unsigned int buf_index = ((j)* nx + i) * 4;

					buffer_data[buf_index + 0] = image->GetData()[ppm_index + 0];
					buffer_data[buf_index + 1] = image->GetData()[ppm_index + 1];
					buffer_data[buf_index + 2] = image->GetData()[ppm_index + 2];
					buffer_data[buf_index + 3] = 1.0f;
				}
			}
		}
		else if (image->GetNumChannels() == 1)
		{
			for (unsigned int i = 0; i < nx; ++i)
			{
				for (unsigned int j = 0; j < ny; ++j)
				{
					unsigned int ppm_index = ((ny - j - 1) * nx + i) * 1;
					unsigned int buf_index = ((j)* nx + i) * 4;

					buffer_data[buf_index + 0] = image->GetData()[ppm_index + 0];
					buffer_data[buf_index + 1] = image->GetData()[ppm_index + 0];
					buffer_data[buf_index + 2] = image->GetData()[ppm_index + 0];
					buffer_data[buf_index + 3] = 1.0f;
				}
			}
		}

		buffer->unmap();

		sampler->setBuffer(0u, 0u, buffer);
		sampler->setFilteringModes(RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_FILTER_NONE);

		return sampler;
	}
}