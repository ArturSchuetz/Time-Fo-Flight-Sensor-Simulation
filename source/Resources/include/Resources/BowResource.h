#pragma once
#include "Resources/Resources_api.h"
#include "Resources/BowResourcesPredeclares.h"

#include <string>

namespace bow
{
	class RESOURCES_API Resource
	{
	public:
		/// Enum identifying the loading state of the resource
		enum class LoadingState : char
		{
			LOADSTATE_UNLOADED = 0,
			LOADSTATE_LOADED = 2,
			LOADSTATE_PREPARED = 4
		};

		Resource(ResourceManager* creator, const std::string& name, ResourceHandle handle);
		virtual ~Resource();

		virtual void VPrepare(void);
		virtual void VLoad(void);
		virtual void VUnload(void);

		virtual ResourceManager* VGetCreator(void) { return m_creator; }

		virtual const std::string& VGetName(void) const { return m_name; }
		virtual ResourceHandle VGetHandle(void) const { return m_handle; }

		virtual size_t VGetSizeInBytes(void) const { return m_sizeInBytes; }

		virtual LoadingState VGetLoadingState() const { return m_loadingState; }
		virtual bool VIsPrepared(void) const { return (m_loadingState == LoadingState::LOADSTATE_PREPARED); }
		virtual bool VIsLoaded(void) const { return (m_loadingState == LoadingState::LOADSTATE_LOADED); }
		virtual void VSetToLoaded(void) { (m_loadingState = LoadingState::LOADSTATE_LOADED); }

		virtual size_t getStateCount() const { return m_stateCount; }

		virtual void _dirtyState();

	protected:
		Resource() : m_creator(0), m_handle(0), m_sizeInBytes(0), m_loadingState(LoadingState::LOADSTATE_UNLOADED) { }

		virtual void VPreLoadImpl(void) {}

		virtual void VPostLoadImpl(void) {}

		virtual void VPreUnloadImpl(void) {}

		virtual void VPostUnloadImpl(void) {}

		virtual void VPrepareImpl(void) {}

		virtual void VUnprepareImpl(void) {}

		virtual void VLoadImpl(void) = 0;

		virtual void VUnloadImpl(void) = 0;

		ResourceManager*	m_creator; /// Creator
		std::string			m_name; /// Unique name of the resource
		ResourceHandle		m_handle; /// Numeric handle for more efficient look up than name
		size_t				m_sizeInBytes; /// The size of the resource in bytes
		LoadingState		m_loadingState; /// Is the resource currently loaded? 
		size_t				m_stateCount; /// State count, the number of times this resource has changed state
	};

} // namespace baselib
