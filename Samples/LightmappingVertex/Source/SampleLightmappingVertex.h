#pragma once

#include "IApplicationEventReceiver.h"

class SampleLightmappingVertex : public IApplicationEventReceiver
{
public:
	SampleLightmappingVertex();
	virtual ~SampleLightmappingVertex();

	virtual void onUpdate();

	virtual void onRender();

	virtual void onPostRender();

	virtual void onResume();

	virtual void onPause();

	virtual bool onBack();

	virtual void onInitApp();

	virtual void onQuitApp();
};