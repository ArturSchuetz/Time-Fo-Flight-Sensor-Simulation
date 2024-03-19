#include <OpenGL3xRenderDevice/Device/Context/FrameBuffer/BowOGL3xColorAttachments.h>
#include <OpenGL3xRenderDevice/Device/Textures/BowOGL3xTexture2D.h>
#include <CoreSystems/BowLogger.h>

#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#endif

namespace bow {

	OGLColorAttachments::OGLColorAttachments()
	{
	}

	OGLColorAttachments::~OGLColorAttachments()
	{
	}

	Texture2DPtr OGLColorAttachments::VGetAttachment(unsigned int index) const
	{
		return std::dynamic_pointer_cast<ITexture2D>(Attachments.at(index).Texture);
	}

	void OGLColorAttachments::VSetAttachment(unsigned int index, Texture2DPtr texture)
	{
		LOG_ASSERT(!((texture != nullptr) && (!texture->VGetDescription().ColorRenderable())), "Texture must be color renderable but the Description.ColorRenderable property is false.");

		if (Attachments.find(index) == Attachments.end())
			Attachments.insert(std::pair<int, OGLColorAttachment>(index, OGLColorAttachment()));

		if (Attachments.at(index).Texture != texture)
		{
			if ((Attachments.at(index).Texture != nullptr) && (texture == nullptr))
			{
				--m_count;
			}
			else if ((Attachments.at(index).Texture == nullptr) && (texture != nullptr))
			{
				++m_count;
			}

			Attachments.at(index).Texture = std::dynamic_pointer_cast<OGLTexture2D>(texture);
			Attachments.at(index).Dirty = true;
			IsDirty = true;
		}
	}

	int OGLColorAttachments::GetCount() const
	{
		return m_count;
	}

}
