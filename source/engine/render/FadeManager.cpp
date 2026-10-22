#include "render/FadeManager.h"

namespace Protean
{

	void FadeManager::Clear()
	{
		opacity = 0;
		state = FADE_FINISHED;
	}

	void FadeManager::Update(float dt)
	{
		fadeEventTriggeredThisFrame = false;
		if (!IsActive())
			return;

		if (targetFadeTimeInSeconds <= 0)
		{
			// skip fade, just launch the job
			onFade();
			state = FADE_FINISHED;
			fadeEventTriggeredThisFrame = true;
		}

		float t = timer / targetFadeTimeInSeconds;
		timer += Protean::Math::Clamp(dt, 0, 0.033f);

		if (state == FADE_IN)
		{
			opacity = Protean::Math::Lerp(0.0f, 1.0f, t);
			if (t >= 1.0f)
			{
				state = FADE_MID;
				opacity = 1.0f;
			}
		}
		else if (state == FADE_MID)
		{
			state = FADE_OUT;
			opacity = 1.0f;
			onFade();
			timer = 0;
			fadeEventTriggeredThisFrame = true;
		}
		else if (state == FADE_OUT)
		{
			opacity = Protean::Math::Lerp(1.0f, 0.0f, t);
			if (t >= 1.0f)
			{
				state = FADE_FINISHED;
				opacity = 0.0f;
			}
		}
		else if (state == FADE_FINISHED)
		{
			opacity = 0.0f;
			onFade = [] {};
		}

	}

}
