#pragma once
#include "Resources/Resources_api.h"
#include "Resources/BowResourcesPredeclares.h"

#include "Resources/Resources/BowImage.h"

namespace bow {

	class ImageLoader_bmp
	{
	public:
		ImageLoader_bmp();
		~ImageLoader_bmp();

		void ImportImage(const char* inputData, Image* outputImage);

	private:
	};
}