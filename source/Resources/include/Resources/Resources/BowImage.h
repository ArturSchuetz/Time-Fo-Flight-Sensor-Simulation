#pragma once
#include "Resources/Resources_api.h"
#include "Resources/BowResourcesPredeclares.h"

#include "Resources/BowResource.h"

#include <vector>

namespace bow
{
	class RESOURCES_API Image : public Resource
	{
		friend class ImageLoader_bmp;
		friend class ImageLoader_hdr;
		friend class ImageLoader_png;
		friend class ImageLoader_tga;

	public:
		Image(ResourceManager* creator, const std::string& name, ResourceHandle handle);
		~Image();

		unsigned int GetHeight();
		unsigned int GetWidth();
		unsigned int GetBitsPerPixel();
		unsigned int GetNumChannels();
		float*		 GetData();

	private:
		/** Loads the image from disk.  This call only performs IO, it
		does not parse the bytestream or check for any errors therein.
		You have to call load() to do that.
		*/
		void VPrepareImpl(void);

		/** Destroys data cached by prepareImpl.
		*/
		void VUnprepareImpl(void);

		/// @copydoc Resource::VLoadImpl
		void VLoadImpl(void);

		/// @copydoc Resource::VPostLoadImpl
		void VPostLoadImpl(void);

		/// @copydoc Resource::VUnloadImpl
		void VUnloadImpl(void);

		char* m_dataFromDisk;

		int	m_width;
		int	m_height;
		short m_numberOfChannels;
		short m_numBitsPerPixel;
		std::vector<float> m_data;
	};

} // namespace bow
