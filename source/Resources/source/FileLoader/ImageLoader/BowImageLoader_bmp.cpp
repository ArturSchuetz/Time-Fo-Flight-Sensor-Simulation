#include "Resources/FileLoader/ImageLoader/BowImageLoader_bmp.h"
#include "Resources/BowResources.h"

namespace bow {

	ImageLoader_bmp::ImageLoader_bmp()
	{

	}

	ImageLoader_bmp::~ImageLoader_bmp()
	{

	}

	void ImageLoader_bmp::ImportImage(const char* inputData, Image* outputImage)
	{
		int headerSize;
		headerSize = *(int*)&inputData[14];
		outputImage->m_width = *(int*)&inputData[18];
		outputImage->m_height = *(int*)&inputData[22];
		outputImage->m_numBitsPerPixel = *(int*)&inputData[28];

		outputImage->m_numberOfChannels = (outputImage->m_numBitsPerPixel / (sizeof(unsigned char) * 8));
		outputImage->m_sizeInBytes = outputImage->m_numberOfChannels * sizeof(float) * outputImage->m_width * outputImage->m_height;
		outputImage->m_data.resize(outputImage->m_numberOfChannels * outputImage->m_width * outputImage->m_height);

		const char* rgb_data = inputData + headerSize;
		for (unsigned int i = 0; i + outputImage->m_numberOfChannels - 1 < (unsigned int)(outputImage->m_numberOfChannels * outputImage->m_width * outputImage->m_height); i += outputImage->m_numberOfChannels)
		{
			for (unsigned int c = 0; c < outputImage->m_numberOfChannels; c++)
			{
				outputImage->m_data[i + (outputImage->m_numberOfChannels - c)] = (float)rgb_data[i + c] / 255.0f;
			}
		}
	}
}
