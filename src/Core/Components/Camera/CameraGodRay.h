#pragma once

#include "Utilities/ECS/Component.h"

namespace MxEngine
{
	class CameraGodRay 
	{
		MAKE_COMPONENT(CameraGodRay);
		float godRayMaxSteps = 150.0f;
		float godRaySampleStep = 0.15f;
		float godRayStepIncrement = 1.01f;
	public:
		CameraGodRay() = default;
		float GetGodRayMaxSteps()const;
		float GetGodRayStepIncrement()const;
		float GetGodRaySampleStep()const;
		void SetGodRayMaxSteps(float num);
		void SetGodRayStepIncrement(float value);
		void SetGodRaySampleStep(float value);
	};
}