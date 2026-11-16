#pragma once
#include <string>
#include <vector>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include <map>

struct IDirect3DDevice9;
struct IDirect3DTexture9;
struct ID3D11ShaderResourceView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;

class c_variables
{
public:
	struct
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground;
		ImVec2 size = ImVec2(1024, 720);
		ImVec2 padding = ImVec2(0, 0);
		ImVec2 spacing = ImVec2(10, 10);
		float shadow_size = 0.f;
		float border_size = 0.f;
		float rounding = 0.f;
		ImVec2 logo_size = ImVec2(160, 52);
		ImVec2 section_size = ImVec2(160, 518);
		ImVec2 child_select_size = ImVec2(270, 80);
		ImVec2 gui_dpi_size = ImVec2(255, 40);
		ImVec2 hue_slider_size = ImVec2(370, 30);
		ImVec2 topbar_size = ImVec2(size.x - section_size.x - spacing.x, 30);
		float scrollbar_size = 4.f;
	} window;

	struct
	{
		ImFont* inter[4];
		ImFont* icons[10];
	} font;

	struct
	{
		bool enabled = false;
		float content_alpha = 0.f;

		int section_count = 0;
		const char* section_icons[5] = { "COMBAT", "VISUALS", "NAVIGATION", "MISC", "SETTINGS" };

		bool favorite_childs = false;
		std::map<ImGuiID, bool> child_states;

		std::string app_name = "svc";
		std::string app_year = "svc";

		ID3D11ShaderResourceView* logo_resource = nullptr;
	} gui;

	struct {

		struct {
			bool enabled = false;
			bool draw_fov = false;
			bool target_marker = false;
			int bone = 0;
			int mode = 0;
			int target_selection = 0;
			int key = 0x02;
			float fov = 20.f;
			int smooth = 5;
			float sensitivity = 1.0f;
			bool sticky_target = false;
			float humanization_bezier = 0.0f;
			int game_mode = 0;
			bool friendly_fire = false;
			bool visible_only = true;
			bool rcs_enabled = false;
			int rcs_strength = 50;
		} combat;

		struct {
			bool enabled = false;
			int key = 0x06;
			int bone = 0;
			int mode = 0;
			int delay_shot = 100;
			int post_shot_delay = 50;
			float hitchance = 80.0f;
		} trigger_control;

		struct {
		bool enabled = true;
			bool agent_name = true;
			bool weapon = true;
			bool skeleton = true;
			bool box = true;
			bool box3d = false;
			bool rainbow = false;
			bool china_hat = false;
			bool show_fov = false;
			float fov_radius = 150.0f;
			float fov_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
			bool radar = false;
			float radar_range = 60.0f;
			float radar_size = 220.0f;
			bool view_angle = false;
			bool ignore_dormant = false;
			bool show_all_bones = false;
			bool lines = true;
			bool head = true;
			bool health = true;
			bool distance = true;
			bool visible_check = true;
			bool show_bots = true;
			bool team_check = false;
			bool objective = true;
			float not_visible_color[4] = { 1.f, 0.0f, 0.0f, 1.f };
			float visible_color[4] = { 0.0f, 1.0f, 0.35f, 1.f };

			bool skills = true;
			bool skills_name = true;
			bool skills_distance = true;
			float skills_color[4] = { 1.f, 0.0f, 0.0f, 1.f };

			bool nav_helper = true;
			float nav_display_distance = 6.0f;
			float nav_aim_guide_distance = 1.5f;
			float nav_position_tolerance = 1.0f;
			float nav_crosshair_tolerance = 1.3f;

			bool timing_bar = true;
			float timing_charge_time = 1.5f;
			float timing_bar_width = 220.0f;
			float timing_bar_height = 14.0f;
			bool timing_sound = false;
			bool streamer_mode = true;

			float box_thickness = 1.2f;
			float skeleton_thickness = 1.2f;
			float lines_thickness = 1.0f;
			float box_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			float skeleton_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			float lines_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			float china_hat_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };




			int text_position = 0;
		} visuals;

	} config;

	float dpi = 1.f;
	int stored_dpi = 100;
	bool dpi_changed = true;

	IDirect3DDevice9* device_dx9 = nullptr;

	ID3D11Device* device_dx11 = nullptr;
	ID3D11DeviceContext* device_context = nullptr;
	IDXGISwapChain* swap_chain = nullptr;
};

inline c_variables* var = new c_variables();
