#include <common.hpp>
#include <core/iapplication.hpp>
#include <d3d11.h>
#include <render/settings/functions.h>
#include <render/resource.h>
#include <render/render.hpp>

constexpr int brand_icon_width = 64;
constexpr int brand_icon_height = 69;

namespace {
ID3D11ShaderResourceView* maxi_icon_texture()
{
	if (var->gui.logo_resource || !var->device_dx11)
		return var->gui.logo_resource;


    HRSRC hRes = FindResourceW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDR_IMAGE_BRAND_ICON), RT_RCDATA);
    if (!hRes) return nullptr;
    HGLOBAL hData = LoadResource(GetModuleHandleW(nullptr), hRes);
    if (!hData) return nullptr;
    void* brand_icon_rgba = LockResource(hData);


	D3D11_TEXTURE2D_DESC description{};
	description.Width = static_cast<UINT>(brand_icon_width);
	description.Height = static_cast<UINT>(brand_icon_height);
	description.MipLevels = 1;
	description.ArraySize = 1;
	description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	description.SampleDesc.Count = 1;
	description.Usage = D3D11_USAGE_IMMUTABLE;
	description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA pixels{};
	pixels.pSysMem = brand_icon_rgba;
	pixels.SysMemPitch = static_cast<UINT>(brand_icon_width * 4);

	ID3D11Texture2D* texture = nullptr;
	if (SUCCEEDED(var->device_dx11->CreateTexture2D(&description, &pixels, &texture))) {
		var->device_dx11->CreateShaderResourceView(texture, nullptr, &var->gui.logo_resource);
		texture->Release();
	}

	return var->gui.logo_resource;
}
}

void c_gui::render()
{
	if (!var->gui.enabled)
		return;

	if (var->gui.app_year == "svc") {
		var->gui.app_year = xorstr_("MAXI WIN XP");
		var->gui.app_name = xorstr_("MAXI WIN XP");
	}

	notify->setup_notify();

	static int active_tab = 0;

	gui->set_next_window_size(SCALE(var->window.size), ImGuiCond_Once);
	gui->set_next_window_pos(ImGui::GetIO().DisplaySize / 2 - SCALE(var->window.size) / 2, ImGuiCond_Once);
	gui->begin(xorstr_("menu"), nullptr, var->window.flags, 0, 0, 0, 0);
	{
		const ImVec2 pos = GetWindowPos();
		const ImVec2 size = GetWindowSize();
		ImDrawList* drawlist = GetWindowDrawList();
		ImGuiStyle* style = &GetStyle();
		style->WindowPadding = SCALE(var->window.padding);
		style->ItemSpacing = SCALE(var->window.spacing);
		style->WindowRounding = 0.0f;
		style->ChildRounding = 0.0f;
		style->FrameRounding = 0.0f;
		style->WindowBorderSize = 0.0f;
		style->ChildBorderSize = 0.0f;
		style->ScrollbarSize = SCALE(6.0f);

		const float topbar_height = SCALE(52.0f);
		const float sidebar_width = SCALE(160.0f);
		const float panel_cut = SCALE(22.0f);
		const ImVec2 panel_max = pos + size;

		maxi_chamfer_fill(drawlist, pos, panel_max, panel_cut, draw->get_clr(clr->window.background));
		maxi_chamfer_fill(drawlist, pos, ImVec2(panel_max.x, pos.y + topbar_height), panel_cut, draw->get_clr(clr->window.bar));
		drawlist->AddRectFilled(ImVec2(pos.x, pos.y + topbar_height), ImVec2(pos.x + sidebar_width, panel_max.y), draw->get_clr(clr->window.bar));
		drawlist->AddRectFilled(ImVec2(pos.x, pos.y + topbar_height), ImVec2(panel_max.x, pos.y + topbar_height + SCALE(1.0f)), draw->get_clr(clr->child.line));
		drawlist->AddRectFilled(ImVec2(pos.x + sidebar_width, pos.y + topbar_height), ImVec2(pos.x + sidebar_width + SCALE(1.0f), panel_max.y), draw->get_clr(clr->child.line));
		maxi_chamfer_stroke(drawlist, pos, panel_max, panel_cut, draw->get_clr(clr->window.border), SCALE(1.0f));

		const ImVec2 icon_min = pos + SCALE(16.0f, 10.0f);
		const float icon_width = SCALE(28.0f);
		const float icon_height = icon_width * static_cast<float>(brand_icon_height) / static_cast<float>(brand_icon_width);
		if (ID3D11ShaderResourceView* icon = maxi_icon_texture())
			drawlist->AddImage(icon, icon_min, icon_min + ImVec2(icon_width, icon_height));

		ImFont* brand_font = var->font.inter[3] ? var->font.inter[3] : GetFont();
		const ImVec2 brand_pos = pos + SCALE(53.0f, 19.0f);
		drawlist->AddText(brand_font, SCALE(13.0f), brand_pos, draw->get_clr(clr->window.name), "MAXI");
		const float maxi_width = brand_font->CalcTextSizeA(SCALE(13.0f), FLT_MAX, 0.0f, "MAXI").x;
		drawlist->AddText(brand_font, SCALE(13.0f), brand_pos + ImVec2(maxi_width, 0.0f), draw->get_clr(clr->accent), "WIN XP");

		draw->render_text(drawlist, var->font.inter[2], pos + ImVec2(sidebar_width, 0.0f), ImVec2(panel_max.x - SCALE(20.0f), pos.y + topbar_height),
			draw->get_clr(clr->window.year), var->gui.app_year.c_str(), NULL, NULL, ImVec2(1.0f, 0.5f));

		gui->push_style_color(ImGuiCol_ChildBg, draw->get_clr(ImVec4(0, 0, 0, 0)));
		gui->set_cursor_pos(SCALE(14.0f, 68.0f));
		gui->begin_content(xorstr_("sections"), SCALE(132.0f, 230.0f), SCALE(elements->section.padding), SCALE(elements->section.spacing), false, false);
		{
			for (int i = 0; i < IM_ARRAYSIZE(var->gui.section_icons); i++)
				gui->section(var->gui.section_icons[i], i, var->gui.section_count);
		}
		gui->end_content();

		const float footer_left = pos.x + SCALE(16.0f);
		const float footer_right = pos.x + sidebar_width - SCALE(14.0f);
		const float footer_top = panel_max.y - SCALE(72.0f);
		drawlist->AddRectFilled(ImVec2(footer_left, footer_top), ImVec2(footer_right, footer_top + SCALE(1.0f)), draw->get_clr(clr->child.line));
		draw->render_text(drawlist, var->font.inter[2], ImVec2(footer_left, footer_top + SCALE(10.0f)), ImVec2(footer_right, footer_top + SCALE(31.0f)), draw->get_clr(clr->window.year), "BUILD", NULL, NULL, ImVec2(0.0f, 0.5f));
		draw->render_text(drawlist, var->font.inter[2], ImVec2(footer_left, footer_top + SCALE(10.0f)), ImVec2(footer_right, footer_top + SCALE(31.0f)), draw->get_clr(clr->window.name), "1.0", NULL, NULL, ImVec2(1.0f, 0.5f));
		draw->render_text(drawlist, var->font.inter[2], ImVec2(footer_left, footer_top + SCALE(32.0f)), ImVec2(footer_right, footer_top + SCALE(53.0f)), draw->get_clr(clr->window.year), "STATUS", NULL, NULL, ImVec2(0.0f, 0.5f));
		draw->render_text(drawlist, var->font.inter[2], ImVec2(footer_left, footer_top + SCALE(32.0f)), ImVec2(footer_right, footer_top + SCALE(53.0f)), draw->get_clr(clr->accent), "READ-ONLY ESP", NULL, NULL, ImVec2(1.0f, 0.5f));





		{
			const char* overlay_src = "none";
			bool ov_active = false;
			if (maxi::render::m_render) {
				overlay_src = maxi::render::m_render->get_overlay_source();
				ov_active = maxi::render::m_render->is_overlay_active();
			}
			if (overlay_src && !overlay_src[0]) overlay_src = "none";
			draw->render_text(drawlist, var->font.inter[2], ImVec2(footer_left, footer_top + SCALE(54.0f)), ImVec2(footer_right, footer_top + SCALE(75.0f)), draw->get_clr(clr->window.year), "OVERLAY", NULL, NULL, ImVec2(0.0f, 0.5f));
			const ImU32 ov_clr = ov_active ? draw->get_clr(clr->accent) : draw->get_clr(ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
			draw->render_text(drawlist, var->font.inter[2], ImVec2(footer_left, footer_top + SCALE(54.0f)), ImVec2(footer_right, footer_top + SCALE(75.0f)), ov_clr, overlay_src, NULL, NULL, ImVec2(1.0f, 0.5f));
		}

		{
			var->gui.content_alpha = ImClamp(var->gui.content_alpha + (gui->fixed_speed(6.f) * (var->gui.section_count == active_tab ? 1.f : -1.f)), 0.f, 1.f);

			if (var->gui.content_alpha == 0.f)
				active_tab = var->gui.section_count;

			gui->set_cursor_pos(ImVec2(sidebar_width + SCALE(18.0f), topbar_height + SCALE(16.0f)));
			gui->begin_content(xorstr_("functional"), ImVec2(size.x - sidebar_width - SCALE(36.0f), size.y - topbar_height - SCALE(32.0f)), SCALE(elements->content.padding), SCALE(elements->content.spacing), true, false);
			{
				gui->push_style_var(ImGuiStyleVar_Alpha, var->gui.content_alpha);

				if (active_tab == 0)
				{
					var->config.combat.enabled = false;
					var->config.trigger_control.enabled = false;
					gui->begin_child(xorstr_("READ-ONLY MOD"), xorstr_("B"));
					{
						ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "ESP READ-ONLY ACTIVE");
						ImGui::Separator();
						ImGui::TextWrapped("Driver access expired; to avoid usermode detection risk,");
						ImGui::TextWrapped("Aimbot / Triggerbot / mouse injection are fully disabled.");
						ImGui::TextWrapped("ESP only (Box/Skeleton/Health/Name) - no writes.");
						ImGui::Spacing();
						ImGui::TextWrapped("For driver support, use your own signed driver (see README).");
					}
					gui->end_child();
					gui->begin_group();
					{
						gui->begin_child(xorstr_("Aimbot Settings (DISABLED)"), xorstr_("B"));
						{
							ImGui::BeginDisabled(true);
							static keybind_state aim_state = { 0, var->config.combat.key, 0 };
							if(gui->checkbox_with_key(xorstr_("Enabled"), &var->config.combat.enabled, &aim_state)) {
								notify->add_notify(std::format("COMBAT {}", var->config.visuals.enabled ? "ENABLED" : "DISABLED"), !var->config.combat.enabled);
							}
							var->config.combat.key = aim_state.key;

							const char* aim_modes[1] = { xorstr_("Memory (Kernel Safe)") };
							gui->dropdown(xorstr_("Aim Mode"), &var->config.combat.mode, aim_modes, IM_ARRAYSIZE(aim_modes));

							const char* target_selections[3] = { xorstr_("FOV"), xorstr_("Distance"), xorstr_("Health") };
							gui->dropdown(xorstr_("Priority"), &var->config.combat.target_selection, target_selections, IM_ARRAYSIZE(target_selections));

							const char* bones[5] = { xorstr_("Head"), xorstr_("Neck"), xorstr_("Chest"), xorstr_("Pelvis"), xorstr_("Multi-Bone") };
							gui->dropdown(xorstr_("Bone"), &var->config.combat.bone, bones, IM_ARRAYSIZE(bones));

							const char* game_modes[2] = { xorstr_("Standard"), xorstr_("Deathmatch (skip vis check)") };
							gui->dropdown(xorstr_("Mode"), &var->config.combat.game_mode, game_modes, IM_ARRAYSIZE(game_modes));

							gui->slider_int(xorstr_("Smoothing"), &var->config.combat.smooth, 0, 50);
							gui->slider_float(xorstr_("Sensitivity"), &var->config.combat.sensitivity, 0.05f, 5.0f, "%.2f");
							gui->checkbox(xorstr_("Line of Sight"), &var->config.combat.visible_only);
							gui->checkbox(xorstr_("Team Check"), &var->config.combat.friendly_fire);
							ImGui::EndDisabled();
						}
						gui->end_child();

						gui->begin_child(xorstr_("Aimbot Display (DISABLED)"), xorstr_("D"));
						{
							ImGui::BeginDisabled(true);
							gui->checkbox(xorstr_("Draw Aim Range"), &var->config.combat.draw_fov);
							gui->checkbox(xorstr_("Marker"), &var->config.combat.target_marker);
							gui->checkbox(xorstr_("Lock Target"), &var->config.combat.sticky_target);
							gui->slider_float(xorstr_("Aim Range"), &var->config.combat.fov, 0.f, 500.f, "%.2f");
							ImGui::EndDisabled();
						}
						gui->end_child();
					}
					gui->end_group();

					gui->sameline();


					gui->begin_group();
					{
						ImGui::BeginDisabled(true);
						gui->begin_child(xorstr_("Triggerbot (DISABLED)"), xorstr_("A"));
						{
							static keybind_state aim_state = { 0, var->config.trigger_control.key, 0 };
							gui->checkbox_with_key(xorstr_("Enabled"), &var->config.trigger_control.enabled, &aim_state);
							var->config.trigger_control.key = aim_state.key;

							const char* trigger_modes[2] = { xorstr_("Default"), xorstr_("Magnet") };
							gui->dropdown(xorstr_("Fire Mode"), &var->config.trigger_control.mode, trigger_modes, IM_ARRAYSIZE(trigger_modes));

							const char* trigger_bones[4] = { xorstr_("Head"), xorstr_("Neck"), xorstr_("Chest"), xorstr_("Pelvis") };
							gui->dropdown(xorstr_("Hit Bone"), &var->config.trigger_control.bone, trigger_bones, IM_ARRAYSIZE(trigger_bones));

							gui->slider_int(xorstr_("Fire Delay"), &var->config.trigger_control.delay_shot, 0, 1000, "%d ms");
							gui->slider_int(xorstr_("Cooldown"), &var->config.trigger_control.post_shot_delay, 0, 1000, "%d ms");
							gui->slider_float(xorstr_("Hit Accuracy"), &var->config.trigger_control.hitchance, 0.f, 100.f, "%.1f%%");
						}
						gui->end_child();

						gui->begin_child(xorstr_("Humanization"), xorstr_("H"));
						{
							gui->slider_float(xorstr_("Curve"), &var->config.combat.humanization_bezier, 0.0f, 10.0f, "%.1f");
						}
						gui->end_child();

						gui->begin_child(xorstr_("Recoil Control (DISABLED)"), xorstr_("R"));
						{
							gui->checkbox(xorstr_("Recoil Control"), &var->config.combat.rcs_enabled);
							if (var->config.combat.rcs_enabled) {
								gui->slider_int(xorstr_("Strength"), &var->config.combat.rcs_strength, 0, 100, "%d%%");
							}
						}
						gui->end_child();
						ImGui::EndDisabled();
					}
					gui->end_group();
				}
				else if (active_tab == 1)
				{
					gui->begin_group();
					{
						gui->begin_child(xorstr_("Players"), xorstr_("F"));
						{
							if (gui->checkbox(xorstr_("Enabled"), &var->config.visuals.enabled)) {
								notify->add_notify(std::format("VISUALS {}", var->config.visuals.enabled ? "ENABLED" : "DISABLED"), !var->config.visuals.enabled);
							}

							gui->checkbox_with_color(xorstr_("Visible check"), &var->config.visuals.visible_check, var->config.visuals.visible_color, var->config.visuals.not_visible_color, true);							gui->checkbox_with_color(xorstr_("Box"), &var->config.visuals.box, var->config.visuals.box_color, true);
							gui->checkbox(xorstr_("Box 3D"), &var->config.visuals.box3d);
							gui->checkbox_with_color(xorstr_("Skeleton"), &var->config.visuals.skeleton, var->config.visuals.skeleton_color, true);
							gui->checkbox(xorstr_("Full Offset Bone"), &var->config.visuals.show_all_bones);
							gui->checkbox_with_color(xorstr_("Lines"), &var->config.visuals.lines, var->config.visuals.lines_color, true);
							gui->checkbox(xorstr_("Name"), &var->config.visuals.agent_name);
							gui->checkbox(xorstr_("Weapon"), &var->config.visuals.weapon);
							gui->checkbox(xorstr_("Head"), &var->config.visuals.head);
							gui->checkbox(xorstr_("Health"), &var->config.visuals.health);
							gui->checkbox(xorstr_("Distance"), &var->config.visuals.distance);
							gui->checkbox(xorstr_("Show Bots"), &var->config.visuals.show_bots);
							gui->checkbox(xorstr_("Rainbow"), &var->config.visuals.rainbow);
							gui->checkbox_with_color(xorstr_("China Hat"), &var->config.visuals.china_hat, var->config.visuals.china_hat_color, true);
							gui->checkbox_with_color(xorstr_("Show FOV"), &var->config.visuals.show_fov, var->config.visuals.fov_color, true);
							gui->slider_float(xorstr_("FOV Radius"), &var->config.visuals.fov_radius, 20.f, 900.f, "%.0f");
							gui->checkbox(xorstr_("Radar"), &var->config.visuals.radar);
							gui->slider_float(xorstr_("Radar Range"), &var->config.visuals.radar_range, 20.f, 150.f, "%.0fm");
							gui->checkbox(xorstr_("View Angle"), &var->config.visuals.view_angle);
							gui->checkbox(xorstr_("Ignore Dormant"), &var->config.visuals.ignore_dormant);
							gui->checkbox(xorstr_("Team Check"), &var->config.visuals.team_check);
							{
								const char* txt_pos[4] = { xorstr_("Bottom"), xorstr_("Top"), xorstr_("Left"), xorstr_("Right") };
								gui->dropdown(xorstr_("Text Position"), &var->config.visuals.text_position, txt_pos, 4);
							}
						}
						gui->end_child();
					}
					gui->end_group();

					gui->sameline();

					gui->begin_group();
					{
						gui->begin_child(xorstr_("Players Styling"), xorstr_("I"));
						{
							gui->slider_float(xorstr_("Box Thickness"), &var->config.visuals.box_thickness, 0.5f, 5.0f, "%.1f px");
							gui->slider_float(xorstr_("Skeleton Thickness"), &var->config.visuals.skeleton_thickness, 0.5f, 5.0f, "%.1f px");
							gui->slider_float(xorstr_("Lines Thickness"), &var->config.visuals.lines_thickness, 0.5f, 5.0f, "%.1f px");
						}
						gui->end_child();

						gui->begin_child(xorstr_("Skills"), xorstr_("I"));
						{
							gui->checkbox_with_color(xorstr_("Skills"), &var->config.visuals.skills, &var->config.visuals.skills_color, true);
							gui->checkbox(xorstr_("Name"), &var->config.visuals.skills_name);
							gui->checkbox(xorstr_("Distance"), &var->config.visuals.skills_distance);
						}
						gui->end_child();
					}
					gui->end_group();
				}
				else if (active_tab == 2)
				{
					gui->begin_group();
					{
						gui->begin_child(xorstr_("Lineups"), xorstr_("M"));
						{
							gui->checkbox(xorstr_("NavPoint Helper"), &var->config.visuals.nav_helper);
							if (var->config.visuals.nav_helper) {
								gui->slider_float(xorstr_("Display Distance"), &var->config.visuals.nav_display_distance, 1.0f, 30.0f, "%.1f m");
								var->config.visuals.nav_aim_guide_distance = std::clamp(
									var->config.visuals.nav_aim_guide_distance, 0.25f, var->config.visuals.nav_display_distance);
								gui->slider_float(xorstr_("Guide Range"), &var->config.visuals.nav_aim_guide_distance, 0.25f, var->config.visuals.nav_display_distance, "%.2f m");
								var->config.visuals.nav_position_tolerance = std::clamp(
									var->config.visuals.nav_position_tolerance, 0.1f, var->config.visuals.nav_aim_guide_distance);
								gui->slider_float(xorstr_("Guide Tolerance"), &var->config.visuals.nav_position_tolerance, 0.1f, var->config.visuals.nav_aim_guide_distance, "%.1f m");
								gui->slider_float(xorstr_("Aim Tolerance"), &var->config.visuals.nav_crosshair_tolerance, 0.5f, 20.0f, "%.1f px");
							}
							gui->checkbox(xorstr_("Timing Bar"), &var->config.visuals.timing_bar);
							if (var->config.visuals.timing_bar) {
								gui->slider_float(xorstr_("Charge Time"), &var->config.visuals.timing_charge_time, 0.3f, 5.0f, "%.1f s");
							}
						}
						gui->end_child();
					}
					gui->end_group();
				}
				else if (active_tab == 3)
				{
					gui->begin_group();
					{
						gui->begin_child(xorstr_("Misc Features"), xorstr_("L"));
						{
							gui->checkbox(xorstr_("Objective"), &var->config.visuals.objective);
							gui->checkbox(xorstr_("Stream Proof Mode"), &var->config.visuals.streamer_mode);
						}
						gui->end_child();
					}
					gui->end_group();

					gui->sameline();

					gui->begin_group();
					{
						gui->begin_child(xorstr_("Configuration"), xorstr_("L"));
						{
							if (gui->button(xorstr_("Save Config")))
								notify->add_notify(std::string(xorstr_("Config saved successfully!")), false);

							if (gui->button(xorstr_("Load Config")))
								notify->add_notify(std::string(xorstr_("Config loaded successfully!")), false);

							if (gui->button(xorstr_("Unload & Exit"))) {
								notify->add_notify(std::string(xorstr_("Unloading... cleaning up driver/DLL")), true);
								maxi::core::request_full_unload();
								PostQuitMessage(0);
							}
						}
						gui->end_child();
					}
					gui->end_group();
				}
				else if (active_tab == 4)
				{
					gui->begin_group();
					{
						gui->begin_child(xorstr_("Credits"), xorstr_("M"));
						{
							static char coder_name[32] = "Sheintan";
							gui->text_field(xorstr_("Coder"), coder_name, sizeof(coder_name), ImGuiInputTextFlags_ReadOnly);
						}
						gui->end_child();
					}
					gui->end_group();
				}
				gui->pop_style_var();
			}
			gui->end_content();

			gui->pop_style_color();
		}
	}
	gui->end();

	static const uint8_t wm_enc[] = { 0x32,0x2B,0x29,0x2E,0x22,0x39,0x28,0x37,0x37,0x36,0x37,0x2B,0x21,0x31,0x3B };
	std::string wm;
	for (int i = 0; i < 15; ++i) wm += static_cast<char>(wm_enc[i] ^ 0x5B);
	draw->watermark(wm.c_str());
}
