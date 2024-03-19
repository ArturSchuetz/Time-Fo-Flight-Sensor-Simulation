#pragma once
#include "Resources/Resources_api.h"
#include "Resources/BowResourcesPredeclares.h"

#include "Resources/Resources/BowImage.h"

namespace bow {

	class ImageLoader_hdr
	{
	public:
		ImageLoader_hdr();
		~ImageLoader_hdr();

		void ImportImage(const char* inputData, Image* outputImage);

	private:
	};
}