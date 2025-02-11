#pragma once

#include "MQRespawnUI.h"

ImVec4 ColorRed = ImVec4(0.990f, 0.148f, 0.148f, 1.0f);
ImVec4 ColorGreen = ImVec4(0.0142f, 0.710f, 0.0490f, 1.0f);
ImVec4 ColorCyan = ImVec4(0.165f, 0.631f, 0.596f, 1.0f);
ImVec4 ColorYellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
ImVec4 ColorMagenta = ImVec4(0.827f, 0.212f, 0.51f, 1.0f);
ImVec4 ColorOrange = ImVec4(0.796f, 0.294f, 0.086f, 1.0f);

static std::map<std::chrono::seconds, ImVec4, std::greater<std::chrono::seconds>> CountDownColorMap = {
	{std::chrono::seconds(600), ColorCyan},
	{std::chrono::seconds(60), ColorYellow},
	{std::chrono::seconds(0), ColorRed},
};

static std::tuple<int, int> ToMinutesAndSeconds(const std::chrono::seconds& time) {
	int minutes = std::chrono::duration_cast<std::chrono::minutes>(time).count();
	int seconds = time.count() % 60;
	return  std::make_tuple(minutes, seconds);
}

static const ImVec4 GetColor(std::chrono::seconds time) {
	for (const auto& colorMap : CountDownColorMap) {
		if (time.count() > colorMap.first.count()) {
			return colorMap.second;
		}
	}

	return ColorOrange;
}

static void RenderSpawnHelpMarker(const std::unordered_map<std::string, u_int>& spawnsMap)
{
	u_int totalCount = std::accumulate(spawnsMap.begin(), spawnsMap.end(), 0u, [](const u_int previous, const std::pair<std::string, u_int>& element) { return previous + element.second; });
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		for (const auto& elem : spawnsMap) {
			ImGui::Text("%s (%i%%)", elem.first.c_str(), (int)(elem.second * 100 / totalCount));
		}
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static void RenderHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static void RenderStatusHelpMarker()
{
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::ColorButton("##up", ColorGreen);
		ImGui::SameLine();
		ImGui::TextUnformatted("Up");

		ImGui::ColorButton("##calculating", ColorMagenta);
		ImGui::SameLine();
		ImGui::TextUnformatted("Down + Calculating");

		for (const auto& colorMap : CountDownColorMap) {
			auto [minutes, seconds] = ToMinutesAndSeconds(colorMap.first);
			char label[32];
			static char text_buf[32] = "";
			sprintf_s(label, "##respawn%im%is", minutes, seconds);
			ImGui::ColorButton(label, colorMap.second);
			ImGui::SameLine();
			if (minutes > 0) {
				if (seconds > 0) {
					ImGui::Text("Respawn > %im %is", minutes, seconds);
				}
				else {
					ImGui::Text("Respawn > %im", minutes);
				}
			}
			else {
				ImGui::Text("Respawn > %is", minutes);
			}
		}

		ImGui::ColorButton("##respawnoverdue", ColorOrange);
		ImGui::SameLine();
		ImGui::TextUnformatted("Respawn overdue");

		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static void RenderTimeColumn(const std::chrono::seconds& time, const ImVec4& color) {
	if (time.count() > 60) {
		auto [minutes, seconds] = ToMinutesAndSeconds(time);
		ImGui::TextColored(color, "%im %is", minutes, seconds);
	}
	else {
		ImGui::TextColored(color, "%is", time);
	}
}

static void RenderStatusColumn(std::vector<RespawnWatch>::iterator& it_watch) {
	if (it_watch->currentSpawn && it_watch->currentSpawn->Type != SPAWN_CORPSE) {
		auto upTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - it_watch->spawnTime.value());
		RenderTimeColumn(upTime, ColorGreen);
	}
	else {
		ImVec4 color;
		std::chrono::seconds remaingTime;
		if (it_watch->respawnTimer.has_value()) {
			remaingTime = std::chrono::duration_cast<std::chrono::seconds>(it_watch->timeOfDeath.value() + it_watch->respawnTimer.value() - std::chrono::system_clock::now());
			color = GetColor(remaingTime);
		}
		else {
			color = ColorMagenta;
			remaingTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - it_watch->timeOfDeath.value());
		}

		RenderTimeColumn(remaingTime, color);
	}

	ImGui::SameLine();
	RenderStatusHelpMarker();
}

static void RenderTimeOfDeathColumn(std::vector<RespawnWatch>::iterator& it_watch)
{
	if (it_watch->timeOfDeath.has_value()) {
		auto timeOfDeathString = CreateTimeStamp(it_watch->timeOfDeath.value());
		ImGui::Text("%s", timeOfDeathString.c_str());
	}
	else {
		ImGui::Text("~");
	}
	ImGui::SameLine();
	if (it_watch->respawnTimer.has_value()) {
		auto [minutes, seconds] = ToMinutesAndSeconds(it_watch->respawnTimer.value());
		char buf[32];
		if (minutes > 0) {
			sprintf_s(buf, "Respawn timer: %im %is", minutes, seconds);
		}
		else {
			sprintf_s(buf, "Respawn timer: %is", seconds);
		}
		RenderHelpMarker(buf);
	}
}

void RenderUI(std::vector<RespawnWatch>& respawnWatches, bool* p_open) {
	if (ImGui::Begin("Respawn Watch List", p_open, 0))
	{
		auto disableButton = !pTarget || !pTarget->SpawnID || pTarget->Type != SPAWN_NPC;
		if (disableButton) {
			ImGui::BeginDisabled();
		}

		if (ImGui::SmallButton(ICON_MD_ADD))
		{
			AddTargetToWatchList(pTarget);
		}

		if (disableButton) {
			ImGui::EndDisabled();
		}

		if (pTarget) {
			if (pTarget->SpawnID) {
				ImGui::SameLine();
				ImGui::Text("%s", pTarget->DisplayedName);
				char buf[32];
				sprintf_s(buf, "x: %.3f  y: %.3f  z: %.3f", pTarget->X, pTarget->Y, pTarget->Z);
				ImGui::SameLine();
				RenderHelpMarker(buf);
			}
		}

		ImGui::Separator();

		if (ImGui::BeginTable("##WatchList", 5, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable))
		{
			ImGui::TableSetupColumn("Last spawn", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Location", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Time of death", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("##Action", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();


			for (auto& it_watch = respawnWatches.begin(); it_watch != respawnWatches.end();)
			{
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				ImGui::Text("%s", it_watch->previousSpawnName.c_str());
				ImGui::SameLine();
				RenderSpawnHelpMarker(it_watch->spawnNames);

				ImGui::TableNextColumn();
				ImGui::Text("x: %.3f  y: %.3f  z: %.3f", it_watch->spawnPoint.X, it_watch->spawnPoint.Y, it_watch->spawnPoint.Z);

				ImGui::TableNextColumn();
				RenderStatusColumn(it_watch);

				ImGui::TableNextColumn();
				RenderTimeOfDeathColumn(it_watch);

				ImGui::TableNextColumn();
				if (ImGui::SmallButton(ICON_MD_DELETE))
				{
					it_watch = respawnWatches.erase(it_watch);
				}
				else {
					it_watch++;
				}
			}

			ImGui::EndTable();
		}
	}
	ImGui::End();
}
