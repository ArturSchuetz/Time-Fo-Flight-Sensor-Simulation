#pragma once
#include "Resources/Resources_api.h"
#include "Resources/BowResourcesPredeclares.h"

#include "Resources/Resources/BowImage.h"

namespace bow {

	class ImageLoader_png
	{
	public:
		ImageLoader_png();
		~ImageLoader_png();

		void ImportImage(const std::vector<unsigned char>& inputData, Image* outputImage);

	private:
	};
}