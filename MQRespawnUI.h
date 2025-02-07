#pragma once
#include <mq/Plugin.h>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/ImGuiUtils.h"

#include "MQRespawn.h"

void RenderUI(std::vector<RespawnWatch>& RespawnWatches, bool* p_open);
