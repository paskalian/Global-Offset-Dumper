#pragma once

#include "..\Globals\Globals.h"
#include "..\Process\Process.h"
#include "..\Handler\Handler.h"

namespace GlobalOffsetDumper
{
	void CloseAllMenus();

	void Render();
	void RenderSelectProcess();
	void RenderAbout();
	void RenderExit();
	void RenderConfiguration();
}
