#include <Windows.h>
#include "../../sdkapi.h"
#include <string>
#include <vector>
#include "SmiteGH.h"
#include <algorithm>

PSDK_CONTEXT SDK_CONTEXT_GLOBAL;
void* localPlayer = NULL;
int smiteSlot;
bool iShowWindow = false;
std::vector<std::string> mobs_to_attack;
bool _g_isEnabled = true;
BOOL
WINAPI
DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD fdwReason,
	_In_ LPVOID lpvReserved
)
{
	UNREFERENCED_PARAMETER(hinstDLL);


	if (fdwReason != DLL_PROCESS_ATTACH)
		return TRUE;

	SDK_EXTRACT_CONTEXT(lpvReserved);
	if (!SDK_CONTEXT_GLOBAL)
		return FALSE;




	if (!SDKSTATUS_SUCCESS(SdkNotifyLoadedModule("SmiteGH", SDK_VERSION)))
	{
		return FALSE;
	}

	SdkGetLocalPlayer(&localPlayer);

	for (uint8_t i = 0; i < SPELL_SLOT_MAX; ++i)
	{
		SDK_SPELL Spell;
		SdkGetAISpell(localPlayer, i, &Spell);

		if (strcmp(Spell.DisplayName, "game_spell_displayname_SummonerSmite") == 0)
		{
			smiteSlot = Spell.Slot;
			break;
		}
	}
	if (smiteSlot == 0)
		return false;

	SdkUiConsoleWrite("Found Smite SLot: %d", smiteSlot);

	SdkRegisterOverlayScene(DrawOverlayScene, NULL);

	SdkRegisterGameScene(DrawGameScene, NULL);

	return TRUE;
}
bool exists(std::string const & user_input,
	std::vector<std::string> const & words)
{
	for (int i = 0; i < words.size(); i++)
		if (user_input == words[i])
			return true;
	return false;
}
void
__cdecl
DrawOverlayScene(
	_In_		void* UserData
)
{
	UNREFERENCED_PARAMETER(UserData);
	Draw();
}
void Draw()
{
	static bool isEnabled = true;
	SdkUiCheckbox("Enabled", &isEnabled, NULL);

	bool Expanded = false;
	SdkUiBeginTree("Mobs To Smite", &Expanded);
	if (Expanded)
	{
		setupMobMenuCheckBox("Baron", "SRU_Baron");
		SdkUiForceOnSameLine();
		setupMobMenuCheckBox("Rift Herald", "SRU_RiftHerald");

		std::vector<std::string> Dragons;
		Dragons.emplace_back("SRU_Dragon_Elder");
		Dragons.emplace_back("SRU_Dragon_Air");
		Dragons.emplace_back("SRU_Dragon_Earth");
		Dragons.emplace_back("SRU_Dragon_Fire");
		Dragons.emplace_back("SRU_Dragon_Water");
		setupMultiMobMenuCheckBox("All Dragons", &Dragons);

		setupMobMenuCheckBox("Blue Buff", "SRU_Blue");
		SdkUiForceOnSameLine();
		setupMobMenuCheckBox("Red Buff", "SRU_Red");

		setupMobMenuCheckBox("Gromp", "SRU_Gromp");
		SdkUiForceOnSameLine();
		setupMobMenuCheckBox("Murk Wolfs", "SRU_Murkwolf");

		setupMobMenuCheckBox("Krug", "SRU_Krug");
		SdkUiForceOnSameLine();
		setupMobMenuCheckBox("Crab", "Sru_Crab");

		setupMobMenuCheckBox("Raptor", "SRU_Razorbeak");
		SdkUiEndTree();
	}
	SdkSetSettingBool("Enabled", &isEnabled);
	_g_isEnabled = isEnabled;
}
void
__cdecl
DrawGameScene(
	_In_		void* UserData
)
{
	UNREFERENCED_PARAMETER(UserData);

	SdkEnumGameObjects(AwObjectLoop, NULL);
}
bool
__cdecl
AwObjectLoop(
	_In_		     void* Object,
	_In_opt_		     void* UserData
)
{
	UNREFERENCED_PARAMETER(UserData);

	if (!_g_isEnabled)
		return true;

	if (!SDKSTATUS_SUCCESS(SdkIsObjectMinion(Object)))
		return true;

	bool bDead;
	SdkIsObjectDead(Object, &bDead);
	if (bDead)
		return true;

	bool bInFogOfWar;
	SdkIsUnitVisible(Object, &bInFogOfWar);
	if (!bInFogOfWar)
		return true;

	SDKVECTOR Position;
	SdkGetObjectPosition(Object, &Position);


	SmiteProcess(Object, Position);

	return true;
}

void
SmiteProcess(
	_In_		     void* Object,
	_In_		     SDKVECTOR& Position
)
{
	const char* mobName;
	SdkGetAIName(Object, &mobName);
	if (!exists(mobName, mobs_to_attack))
		return;

	SDK_HEALTH Health;
	SdkGetUnitHealth(Object, &Health);
	bool canCast = false;
	SdkCanAICast(Object, &canCast);

	if (canCast && Health.Current <= GetSmiteDamage())
	{
		//SdkUiConsoleWrite("Can Attack %d", Health.Current);
		SdkCastSpellLocalPlayer(Object, NULL, (unsigned char)smiteSlot, SPELL_CAST_START);
	}
}

int GetSmiteDamage()
{
	int Level = 1;
	if (!SDKSTATUS_SUCCESS(SdkGetHeroExperience(localPlayer, NULL, &Level)))
	{
		SdkUiConsoleWrite("Couldn't get level..");
		_g_isEnabled = false;
		return 0;
	}
	int CalcSmiteDamage[] = {
		20 * Level + 370,
		30 * Level + 330,
		40 * Level + 240,
		50 * Level + 100
	};
	return *std::max_element(CalcSmiteDamage, CalcSmiteDamage + 4);
}

void setupMobMenuCheckBox(const char* optionStr, const char* mobName)
{
	bool static isChecked = true;
	SdkGetSettingBool(optionStr, &isChecked, true);
	SdkUiCheckbox(optionStr, &isChecked, NULL);
	if (isChecked)
	{
		const std::string mob = mobName;
		if (!exists(mob, mobs_to_attack))
			mobs_to_attack.push_back(mob);
	}
	else
	{
		const std::string mob = mobName;
		if (exists(mob, mobs_to_attack))
		{
			const auto result = find(mobs_to_attack.begin(), mobs_to_attack.end(), mob);
			mobs_to_attack.erase(result);
		}
	}
	SdkSetSettingBool(optionStr, isChecked);
}

void setupMultiMobMenuCheckBox(const char* optionStr, std::vector<std::string> mobNames[])
{
	bool static isChecked = true;
	SdkGetSettingBool(optionStr, &isChecked, true);
	SdkUiCheckbox(optionStr, &isChecked, NULL);
	for (auto i = 0; i < mobNames->size(); i++)
	{
		if (isChecked)
		{
			const std::string mob = mobNames->at(i);
			if (!exists(mob, mobs_to_attack))
				mobs_to_attack.push_back(mob);

		}
		else
		{
			const std::string mob = mobNames->at(i);
			if (exists(mob, mobs_to_attack))
			{
				const auto result = find(mobs_to_attack.begin(), mobs_to_attack.end(), mob);
				mobs_to_attack.erase(result);
			}
		}
	}
	SdkSetSettingBool(optionStr, isChecked);
}