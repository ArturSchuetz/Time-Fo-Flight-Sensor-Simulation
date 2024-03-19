#pragma once
#include "Resources/Resources_api.h"
#include "Resources/BowResourcesPredeclares.h"

#include "Resources/Resources/BowImage.h"

namespace bow {

	class ImageLoader_tga
	{
	public:
		ImageLoader_tga();
		~ImageLoader_tga();

		void ImportImage(const char* inputData, Image* outputImage);

	private:
		void LoadCompressedTGA(const char* inputData, Image* outputImage);

		unsigned int m_readCounter;
	};
}