#pragma once

#include <mq/Plugin.h>

struct RespawnWatch {
	std::optional<std::chrono::seconds> respawnTimer;
	CVector3 spawnPoint;
	std::unordered_map<std::string, u_int> spawnNames;
	std::optional<std::chrono::system_clock::time_point> timeOfDeath;
	std::optional<std::chrono::system_clock::time_point> spawnTime;
	std::string previousSpawnName;
	PlayerClient* currentSpawn;
};

std::string CreateTimeStamp(std::chrono::system_clock::time_point time_point);
void AddTargetToWatchList(ForeignPointer<PlayerClient> pTarget);