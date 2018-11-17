#pragma once
#include <sal.h>


void 
__cdecl 
DrawGameScene(
	_In_ void* UserData
);

bool 
__cdecl 
AwObjectLoop(
	_In_		     void* Object,
	_In_opt_         unsigned int NetworkID,
	_In_opt_		     void* UserData
);
 
void 
SmiteProcess(
	_In_ void* Object, 
	_In_ SDKVECTOR& Position
);

int 
GetSmiteDamage();

void
__cdecl
DrawOverlayScene(
	_In_ void* UserData
);

void Draw();
void setupMobMenuCheckBox(const char* optionStr, const char* mobName, bool loadSettings = false);
void setupMultiMobMenuCheckBox(const char* optionStr, std::vector<std::string> mobNames[], bool loadSettings = false);
void LoadSettings();
float RangeDistance(SDKVECTOR PlayerVec, SDKVECTOR MobVec);