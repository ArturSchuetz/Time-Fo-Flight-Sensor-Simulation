#include "Resources/FileLoader/ImageLoader/BowImageLoader_png.h"
#include "Resources/BowResources.h"

#include "CoreSystems/BowLogger.h"

#include "LoadPNG/lodepng.h"

namespace bow {

	ImageLoader_png::ImageLoader_png()
	{

	}

	ImageLoader_png::~ImageLoader_png()
	{

	}

	void ImageLoader_png::ImportImage(const std::vector<unsigned char>& inputData, Image* outputImage)
	{
		lodepng::State state; //optionally customize this one

		unsigned int width, height;
		std::vector<unsigned char> image_data;
		unsigned error = lodepng::decode(image_data, width, height, state, inputData);
		if (!error)
		{
			outputImage->m_width = width;
			outputImage->m_height = height;
			unsigned int channels = -1;
			switch (state.info_png.color.colortype)
			{
			case LCT_GREY:
				channels = 1;
				break;
			case LCT_RGB:
				channels = 3;
				break;
			case LCT_PALETTE:
				channels = 1;
				break;
			case LCT_GREY_ALPHA:
				channels = 2;
				break;
			case LCT_RGBA:
				channels = 4;
				break;
			}

			outputImage->m_numberOfChannels = channels;
			outputImage->m_sizeInBytes = outputImage->m_numberOfChannels * sizeof(float) * outputImage->m_width * outputImage->m_height;
			outputImage->m_numBitsPerPixel = sizeof(float) * channels;
			outputImage->m_data.resize(outputImage->m_numberOfChannels * outputImage->m_width * outputImage->m_height);

			for (unsigned int i = 0; i < (unsigned int)(outputImage->m_width * outputImage->m_height); i++)
			{
				for (unsigned int c = 0; c < outputImage->m_numberOfChannels; c++)
				{
					outputImage->m_data[(i * outputImage->m_numberOfChannels) + c] = (float)(image_data[(i * 4) + c]) / 255.0f;
				}
			}
		}
		else
		{
			std::string errorText = std::string("Could not decode PNG: ") + std::string(lodepng_error_text(error));
			LOG_ERROR(errorText.c_str());
		}
	}
}
