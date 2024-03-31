#ifndef CLICKABLE_SCENE
#define CLICKABLE_SCENE

#include "Scene.h"

// Handle mouse click query so that objects can be selected and manipulated by ImGuizmo
class ClickableScene
{
public:
	ClickableScene() = default;
	void Init();
};

#endif