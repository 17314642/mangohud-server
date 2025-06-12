#include <imgui.h>
#include "overlay.h"
#include "client.h"

void draw_metrics() {
    mangohud_message cur_metrics = {};

    {
        std::unique_lock lock(metrics_lock);
        cur_metrics = metrics;
    }

    if (!cur_metrics.num_of_gpus)
        ImGui::Text("Not connected to server.");

    for (uint8_t i = 0; i < cur_metrics.num_of_gpus; i++) {
        if (gpu_list.size() > 0 && gpu_list.find(i) == gpu_list.end())
            continue;

        gpu& gpu = cur_metrics.gpus[i];
        gpu_metrics_system& system = gpu.system_metrics;
        gpu_metrics_process& process = gpu.process_metrics;

        ImGui::Text("GPU %d:", i);
        ImGui::Text("  Process:");
        ImGui::Text("    load                   = %d%%", process.load);
        ImGui::Text("    vram_used              = %.1f GB", process.vram_used);
        ImGui::Text("    gtt_used               = %.1f GB", process.gtt_used);
        ImGui::NewLine();
        ImGui::Text("  System:");
        ImGui::Text("    load                   = %d%%", system.load);
        ImGui::NewLine();
        ImGui::Text("    vram_used              = %.1f GB", system.vram_used);
        ImGui::Text("    gtt_used               = %.1f GB", system.gtt_used);
        ImGui::NewLine();
        ImGui::Text("    memory_total           = %.1f GB", system.memory_total);
        ImGui::Text("    memory_clock           = %d MHz", system.memory_clock);
        ImGui::Text("    memory_temp            = %d °C", system.memory_temp);
        ImGui::NewLine();
        ImGui::Text("    temperature            = %d °C", system.temperature);
        ImGui::Text("    junction_temperature   = %d °C", system.junction_temperature);
        ImGui::NewLine();
        ImGui::Text("    core_clock             = %d MHz", system.core_clock);
        ImGui::Text("    voltage                = %d mV", system.voltage);
        ImGui::NewLine();
        ImGui::Text("    power_usage            = %.1f W", system.power_usage);
        ImGui::Text("    power_limit            = %.1f W", system.power_limit);
        ImGui::NewLine();
        ImGui::Text("    apu_cpu_power          = %.1f W", system.apu_cpu_power);
        ImGui::Text("    apu_cpu_temp           = %d °C", system.apu_cpu_temp);
        ImGui::NewLine();
        ImGui::Text("    is_power_throttled     = %s", system.is_power_throttled    ? "true" : "false");
        ImGui::Text("    is_current_throttled   = %s", system.is_current_throttled  ? "true" : "false");
        ImGui::Text("    is_temp_throttled      = %s", system.is_temp_throttled     ? "true" : "false");
        ImGui::Text("    is_other_throttled     = %s", system.is_other_throttled    ? "true" : "false");
        ImGui::NewLine();
        ImGui::Text("    fan_speed              = %d %s", system.fan_speed, system.fan_rpm ? "RPM" : "%");
        ImGui::NewLine();
    }
}

void draw_overlay() {
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(300, 670), ImGuiCond_Always);
    ImGui::GetIO().FontGlobalScale = 0.6f;

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.f, 0.f, 0.f, 0.4f);

    ImGui::Begin("metrics overlay");
    draw_metrics();
    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();
}
