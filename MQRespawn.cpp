// MQRespawn.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.
#pragma once

#include <mq/Plugin.h>

#include "MQRespawnUI.h"
#include "MQRespawn.h"

#include <map>

PreSetup("MQRespawn");
PLUGIN_VERSION(0.1);

/**
 * Avoid Globals if at all possible, since they persist throughout your program.
 * But if you must have them, here is the place to put them.
 */
static bool ShowMQRespawnWindow = true;

std::vector<RespawnWatch> m_respawn_watches;
u_int m_position_slack = 5;

std::string CreateTimeStamp(std::chrono::system_clock::time_point time_point)
{
	std::time_t timeOfDeath = std::chrono::system_clock::to_time_t(time_point);
	char buff[20];
	std::tm local_tm;
	localtime_s(&local_tm, &timeOfDeath);
	strftime(buff, sizeof(buff), "%H:%M:%S", &local_tm);
	return std::string(buff);
}

static bool IsBetween(const double p, const double v1, const double v2) {
	return v1 <= v2                                // <- v1 and v2 are compared only once
		? v1 <= p && p <= v2
		: v2 <= p && p <= v1;
}

static bool IsInVisinityOf(const CVector3& vec1, const CVector3& vec2) {
	return IsBetween(vec1.X, vec2.X + m_position_slack, vec2.X - m_position_slack)
		&& IsBetween(vec1.Y, vec2.Y + m_position_slack, vec2.Y - m_position_slack)
		&& IsBetween(vec1.Z, vec2.Z + m_position_slack, vec2.Z - m_position_slack)
		;
}

static void HandleCommand(PlayerClient* pChar, PCHAR szLine)
{
	if (szLine && szLine[0] == '\0')
	{
		WriteChatf("\ag[MQRespawn]\ax Usage:");
		WriteChatf("    /respawntimer \at[ui]\ax");
		WriteChatf("    /respawntimer \at[slack] [slack in pixels]\ax");
		return;
	}

	char szArg1[MAX_STRING] = { 0 };
	GetArg(szArg1, szLine, 1);

	if (ci_equals(szArg1, "ui")) {
		ShowMQRespawnWindow = true;
	}
	else if (ci_equals(szArg1, "slack")) {
		m_position_slack = GetUIntFromString(GetArg(szArg1, szLine, 2), 0);
		WriteChatf("\ag[MQRespawn]\ax slack in pixels is now \at%i\ax", m_position_slack);
	}
}

void AddTargetToWatchList(PlayerClient* pTarget) {
	if (pTarget) {
		auto position = CVector3{ pTarget->X, pTarget->Y, pTarget->Z };
		if (!any_of(m_respawn_watches.begin(), m_respawn_watches.end(), [position](RespawnWatch respawnTimerWatch) { return  IsInVisinityOf(respawnTimerWatch.spawnPoint, position); })) {
			RespawnWatch newWatch;
			newWatch.spawnPoint = position;
			newWatch.spawnTime = std::chrono::system_clock::now();
			std::string spawnName(pTarget->DisplayedName);
			newWatch.previousSpawnName = spawnName;
			newWatch.spawnNames[spawnName] = 1;
			newWatch.currentSpawn = GetSpawnByID(pTarget->SpawnID);
			m_respawn_watches.push_back(newWatch);
		}
	}
}

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("MQRespawn::Initializing version %f", MQ2Version);
	AddCommand("/respawntimer", HandleCommand);
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("MQRespawn::Shutting down");
	RemoveCommand("/respawntimer");
}

PLUGIN_API void SetGameState(int GameState)
{
	if (GameState != GAMESTATE_INGAME)
	{
		if (m_respawn_watches.size() > 0) {
			m_respawn_watches.clear();
			WriteChatf("\ag[MQRespawn]\ax not ingame -> clearing watch list.");
		}
	}
}

PLUGIN_API void OnPulse()
{
	for (auto& watch : m_respawn_watches)
	{
		if (watch.currentSpawn && watch.currentSpawn->Type == SPAWN_CORPSE && !watch.timeOfDeath.has_value()) {
			watch.timeOfDeath = std::chrono::system_clock::now();
			WriteChatf("\ag[MQRespawn]\ax \am%s\ax died, setting time of death \at%s\ax...", watch.previousSpawnName.c_str(), CreateTimeStamp(watch.timeOfDeath.value()).c_str());
		}
	}
}

PLUGIN_API void OnAddSpawn(PlayerClient* pNewSpawn)
{
	auto position = CVector3{ pNewSpawn->X, pNewSpawn->Y, pNewSpawn->Z };
	for (auto& watch : m_respawn_watches)
	{
		if (IsInVisinityOf(watch.spawnPoint, position)) {
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - watch.timeOfDeath.value());
			WriteChatf("\ag[MQRespawn]\ax respawn detected for \am%s\ax with respawntimer of \at%is\ax...", pNewSpawn->DisplayedName, diff.count());
			watch.respawnTimer = diff;
			watch.timeOfDeath.reset();
			watch.spawnTime = std::chrono::system_clock::now();
			std::string spawnName(pNewSpawn->DisplayedName);
			watch.previousSpawnName = spawnName;
			watch.spawnNames[spawnName] = watch.spawnNames[spawnName] + 1;
			watch.currentSpawn = pNewSpawn;
		}
	}
}

PLUGIN_API void OnRemoveSpawn(PlayerClient* pSpawn)
{
	for (auto& watch : m_respawn_watches)
	{
		if (watch.currentSpawn == pSpawn) {
			watch.currentSpawn = nullptr;
		}
	}
}

PLUGIN_API void OnBeginZone()
{
	if (m_respawn_watches.size() > 0) {
		m_respawn_watches.clear();
		WriteChatf("\ag[MQRespawn]\ax zoned -> clearing watch list.", m_position_slack);
	}
}

PLUGIN_API void OnUpdateImGui()
{
	if (GetGameState() == GAMESTATE_INGAME)
	{
		if (ShowMQRespawnWindow) {
			RenderUI(m_respawn_watches, &ShowMQRespawnWindow);
		}
	}
}
