#include <Windows.h>
#include "../../sdkapi.h"
#include <string>
#include <vector>
#include "SmiteGH.h"
#include <algorithm>
#include <sstream>

PSDK_CONTEXT SDK_CONTEXT_GLOBAL;
void* localPlayer = NULL;
int smiteSlot;
bool iShowWindow = false;
std::vector<std::string> mobs_to_attack;
bool isEnabled = true;
SDKVECTOR _g_DirectionVector = { 0, 0, 1.f };
SDKCOLOR _g_ColorBlue = { 255, 0, 0, 255 };
SDKCOLOR _g_ColorRed = { 0, 0, 255, 255 };
bool drawRange = true;
int smiteModifiedRange = 570;

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


	for (int i = 4; i < 6; ++i)
	{
		SDK_SPELL Spell;
		SdkGetAISpell(localPlayer, i, &Spell);

		if (strcmp(Spell.ScriptName, "SummonerSmite") == 0)
		{
			smiteSlot = Spell.Slot;
			break;
		}
	}
	if (smiteSlot == 0)
	{
		return false;
	}

	LoadSettings();

	SdkUiConsoleWrite("Found Smite SLot: %d", smiteSlot);
	SdkRegisterOverlayScene(DrawOverlayScene, NULL);
	SdkRegisterGameScene(DrawGameScene, NULL);

	SdkUiConsoleWrite("SmiteGH Loaded!", smiteSlot);
	return TRUE;
}
bool exists(std::string const & user_input,
	std::vector<std::string> const & words)
{
	for (const auto& word : words)
		if (user_input == word)
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

	SdkUiCheckbox("Enabled", &isEnabled, NULL);
	SdkUiCheckbox("Draw Smite Range", &drawRange, NULL);

	bool Expanded = true;
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
	SdkSetSettingBool("Enabled", isEnabled);
	SdkSetSettingBool("Draw Smite Range", drawRange);
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
	_In_opt_         unsigned int NetworkID,
	_In_opt_		     void* UserData
)
{
	UNREFERENCED_PARAMETER(NetworkID);
	UNREFERENCED_PARAMETER(UserData);

	if (!isEnabled)
		return true;

	if (drawRange && localPlayer == Object)
	{
		SDKVECTOR localPlayerPos;
		SdkGetObjectPosition(localPlayer, &localPlayerPos);
		SDK_SPELL smiteSpell;
		SdkGetAISpell(localPlayer, smiteSlot, &smiteSpell);
		float GameTime;
		SdkGetGameTime(&GameTime);
		int CheckForSmite;
		SdkCanAICastSpell(localPlayer, smiteSlot, NULL, &CheckForSmite);
		bool canCast = smiteSpell.CurrentAmmo > 0 && smiteSpell.CooldownExpires <= GameTime && CheckForSmite == SPELL_CAN_CAST_OK;
		if (canCast)
			SdkDrawCircle(&localPlayerPos, smiteModifiedRange, &_g_ColorBlue, 0, &_g_DirectionVector);
		else
			SdkDrawCircle(&localPlayerPos, smiteModifiedRange, &_g_ColorRed, 0, &_g_DirectionVector);
	}

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
	_In_		     void* Mob,
	_In_		     SDKVECTOR& Position
)
{
	const char* mobName;
	SdkGetAIName(Mob, &mobName);
	if (!exists(mobName, mobs_to_attack))
		return;

	SDK_HEALTH Health;
	SdkGetUnitHealth(Mob, &Health);

	SDK_SPELL smiteSpell;
	SdkGetAISpell(localPlayer, smiteSlot, &smiteSpell);
	float GameTime;
	SdkGetGameTime(&GameTime);
	int CheckForSmite;
	bool alreadyCasted;
	SdkCanAICastSpell(localPlayer, smiteSlot, &alreadyCasted, &CheckForSmite);
	bool canCast = smiteSpell.CurrentAmmo > 0 && smiteSpell.CooldownExpires <= GameTime && CheckForSmite == SPELL_CAN_CAST_OK
		&& !alreadyCasted;

	SDKVECTOR posForLocalPlayer;
	SdkGetObjectPosition(localPlayer, &posForLocalPlayer);

	SDKBOX posForBoundingBoxForMob;
	SdkGetObjectBoundingBox(Mob, &posForBoundingBoxForMob);

	float range = RangeDistance(posForLocalPlayer, posForBoundingBoxForMob.Max);

	if (canCast && Health.Current <= GetSmiteDamage() && range <= 500)
		SdkCastSpellLocalPlayer(Mob, NULL, (unsigned char)smiteSlot, SPELL_CAST_START);
}

int GetSmiteDamage()
{
	int Level = 1;
	if (!SDKSTATUS_SUCCESS(SdkGetHeroExperience(localPlayer, NULL, &Level)))
	{
		SdkUiConsoleWrite("Couldn't get level..");
		isEnabled = false;
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

void setupMobMenuCheckBox(const char* optionStr, const char* mobName, bool loadSettings)
{
	bool static isChecked = true;
	SdkGetSettingBool(optionStr, &isChecked, true);
	if (!loadSettings)
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

void setupMultiMobMenuCheckBox(const char* optionStr, std::vector<std::string> mobNames[], bool loadSettings)
{
	bool static isChecked = true;
	SdkGetSettingBool(optionStr, &isChecked, true);
	if (!loadSettings)
		SdkUiCheckbox(optionStr, &isChecked, NULL);
	for (const auto& mobName : *mobNames)
	{
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
	}
	SdkSetSettingBool(optionStr, isChecked);
}

void LoadSettings()
{
	std::vector<std::string> Dragons;
	Dragons.emplace_back("SRU_Dragon_Elder");
	Dragons.emplace_back("SRU_Dragon_Air");
	Dragons.emplace_back("SRU_Dragon_Earth");
	Dragons.emplace_back("SRU_Dragon_Fire");
	Dragons.emplace_back("SRU_Dragon_Water");

	setupMobMenuCheckBox("Baron", "SRU_Baron", true);
	setupMobMenuCheckBox("Rift Herald", "SRU_RiftHerald", true);
	setupMultiMobMenuCheckBox("All Dragons", &Dragons, true);
	setupMobMenuCheckBox("Blue Buff", "SRU_Blue", true);
	setupMobMenuCheckBox("Red Buff", "SRU_Red", true);
	setupMobMenuCheckBox("Gromp", "SRU_Gromp", true);
	setupMobMenuCheckBox("Murk Wolfs", "SRU_Murkwolf", true);
	setupMobMenuCheckBox("Krug", "SRU_Krug", true);
	setupMobMenuCheckBox("Crab", "Sru_Crab", true);
	setupMobMenuCheckBox("Raptor", "SRU_Razorbeak", true);

	SdkGetSettingBool("Enabled", &isEnabled, true);
	SdkGetSettingBool("Draw Smite Range", &drawRange, true);
}

float RangeDistance(SDKVECTOR PlayerVec, SDKVECTOR MobVec)
{
	return sqrt(pow(PlayerVec.x - MobVec.x, 2) + pow(PlayerVec.y - MobVec.y, 2) + pow(PlayerVec.z - MobVec.z, 2));
}