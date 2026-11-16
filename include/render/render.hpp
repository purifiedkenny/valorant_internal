#pragma once

#include <common.hpp>
#include <include/render/settings/functions.h>
#include <include/render/resource.h>


#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_freetype.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include <dwmapi.h>

namespace maxi::render {

	class RENDER {
	public:
		RENDER() = default;
		~RENDER();

		bool init();
		void begin();
		void end();
		void shutdown();
		float get_fps() const;

		void UpdateKeyStates();
		void UpdateMouseState();

		void set_streamer_mode(bool enabled) { m_streamer_mode = enabled; }


		static bool m_imgui_initialized;

		static LRESULT CALLBACK standalone_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		bool m_standalone { false };
		bool m_own_overlay { false };

		HWND get_window_handle() const { return hWnd; }
		ID3D11Device* get_device() const { return device; }
		ID3D11DeviceContext* get_device_context() const { return device_context; }
		const char* get_overlay_source() const { return m_overlay_source; }
		bool is_overlay_active() const { return m_overlay_active; }

		bool is_hook_active() const;

		float Width{ 0.0f };
		float Height{ 0.0f };

    public:
        void AddText(ImVec2 pos, ImU32 color, const char* text, const char* text_end = nullptr) {
            ImGui::GetBackgroundDrawList()->AddText(pos, color, text, text_end);
        }

		void DrawLine(const ImVec2& aPoint1, const ImVec2 aPoint2, ImU32 aColor, const FLOAT aLineWidth) {

			ImGuiIO& io = ImGui::GetIO();
			float screen_width = io.DisplaySize.x;
			float screen_height = io.DisplaySize.y;


			if (!std::isfinite(aPoint1.x) || !std::isfinite(aPoint1.y) ||
				!std::isfinite(aPoint2.x) || !std::isfinite(aPoint2.y)) {
				return;
			}


			const float margin = 100.0f;
			if (aPoint1.x < -margin || aPoint1.x > screen_width + margin ||
				aPoint1.y < -margin || aPoint1.y > screen_height + margin ||
				aPoint2.x < -margin || aPoint2.x > screen_width + margin ||
				aPoint2.y < -margin || aPoint2.y > screen_height + margin) {
				return;
			}


			float dx = aPoint2.x - aPoint1.x;
			float dy = aPoint2.y - aPoint1.y;
			float distance = sqrt(dx * dx + dy * dy);


			const float max_distance = (screen_width + screen_height) * 1.5f;
			if (distance > max_distance) {
				return;
			}

			auto vList = ImGui::GetBackgroundDrawList();
			vList->AddLine(aPoint1, aPoint2, aColor, aLineWidth);
		}

		void DrawSkeletonLine(const ImVec2& from, const ImVec2& to, ImU32 color, float thickness, float curve_factor = 0.12f) {
			ImDrawList* draw = ImGui::GetBackgroundDrawList();


			ImGuiIO& io = ImGui::GetIO();
			float screen_width = io.DisplaySize.x;
			float screen_height = io.DisplaySize.y;


			const float margin = 50.0f;
			if (from.x < -margin || from.x > screen_width + margin ||
				from.y < -margin || from.y > screen_height + margin ||
				to.x < -margin || to.x > screen_width + margin ||
				to.y < -margin || to.y > screen_height + margin) {
				return;
			}


			float dx = to.x - from.x;
			float dy = to.y - from.y;
			float distance = sqrt(dx * dx + dy * dy);


			if (distance < 15.0f) {
				draw->AddLine(from, to, color, thickness);
				return;
			}


			ImVec2 mid = ImVec2((from.x + to.x) * 0.5f, (from.y + to.y) * 0.5f);


			float perpX = -dy / distance;
			float perpY = dx / distance;


			float curve_offset = distance * curve_factor;
			ImVec2 control = ImVec2(
				mid.x + perpX * curve_offset,
				mid.y + perpY * curve_offset
			);


			if (control.x < -margin || control.x > screen_width + margin ||
				control.y < -margin || control.y > screen_height + margin) {

				draw->AddLine(from, to, color, thickness);
				return;
			}


			draw->AddBezierQuadratic(from, control, to, color, thickness, 12);
		}

		void DrawPlayerName(const ImVec2& position, const std::string& name, ImU32 color) {
			ImDrawList* draw = ImGui::GetBackgroundDrawList();


			if (name.empty()) {
				return;
			}


			ImFont* font = ImGui::GetFont();
			if (!font) {
				return;
			}


			ImVec2 text_size = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, name.c_str());


			if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
				!std::isfinite(text_size.x) || !std::isfinite(text_size.y)) {
				return;
			}


			ImVec2 text_pos = ImVec2(
				position.x - (text_size.x * 0.5f),
				position.y
			);


			const float padding = 2.0f;
			ImVec2 bg_min = ImVec2(text_pos.x - padding, text_pos.y - padding);
			ImVec2 bg_max = ImVec2(text_pos.x + text_size.x + padding, text_pos.y + text_size.y + padding);

			draw->AddRectFilled(bg_min, bg_max, IM_COL32(0, 0, 0, 120), 2.0f);


			draw->AddText(font, font->FontSize, text_pos, color, name.c_str());
		}

		void DrawTracerLine(const ImVec2& target_pos, ImU32 color, float thickness = 1.0f) {
			ImDrawList* draw = ImGui::GetBackgroundDrawList();


			ImGuiIO& io = ImGui::GetIO();
			float screen_width = io.DisplaySize.x;
			float screen_height = io.DisplaySize.y;


			if (screen_width <= 0.0f || screen_height <= 0.0f ||
				!std::isfinite(screen_width) || !std::isfinite(screen_height)) {
				return;
			}


			if (!std::isfinite(target_pos.x) || !std::isfinite(target_pos.y)) {
				return;
			}




			if ((target_pos.x >= -10.0f && target_pos.x <= 10.0f) &&
				(target_pos.y >= -10.0f && target_pos.y <= 10.0f)) {
				return;
			}


			const float edge_threshold = 20.0f;
			if ((target_pos.x >= -edge_threshold && target_pos.x <= edge_threshold) ||
				(target_pos.x >= screen_width - edge_threshold && target_pos.x <= screen_width + edge_threshold) ||
				(target_pos.y >= -edge_threshold && target_pos.y <= edge_threshold) ||
				(target_pos.y >= screen_height - edge_threshold && target_pos.y <= screen_height + edge_threshold)) {


				if (target_pos.x < edge_threshold || target_pos.x > screen_width - edge_threshold ||
					target_pos.y < edge_threshold || target_pos.y > screen_height - edge_threshold) {
					return;
				}
			}


			ImVec2 screen_center_bottom = ImVec2(screen_width * 0.5f, screen_height);


			if (!std::isfinite(screen_center_bottom.x) || !std::isfinite(screen_center_bottom.y)) {
				return;
			}


			const float allowed_outside = 20.0f;
			if (target_pos.x < -allowed_outside || target_pos.x > screen_width + allowed_outside ||
				target_pos.y < -allowed_outside || target_pos.y > screen_height + allowed_outside) {
				return;
			}


			float dx = target_pos.x - screen_center_bottom.x;
			float dy = target_pos.y - screen_center_bottom.y;
			float distance = sqrt(dx * dx + dy * dy);


			if (!std::isfinite(distance)) {
				return;
			}


			const float max_distance = sqrt(screen_width * screen_width + screen_height * screen_height) * 0.8f;
			if (distance > max_distance || distance < 10.0f) {
				return;
			}



			for (float t = 0.1f; t <= 0.9f; t += 0.2f) {
				float check_x = screen_center_bottom.x + (dx * t);
				float check_y = screen_center_bottom.y + (dy * t);


				if ((check_x >= -10.0f && check_x <= 10.0f && check_y >= -10.0f && check_y <= 10.0f) ||
					check_x < -allowed_outside || check_y < -allowed_outside ||
					check_x > screen_width + allowed_outside || check_y > screen_height + allowed_outside) {
					return;
				}
			}


			draw->AddLine(screen_center_bottom, target_pos, color, thickness);
		}

		void HealthBar(float x, float y, float w, float h, int value, int v_max)
		{
			ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), ImColor(0.f, 0.f, 0.f, 0.725f), 0.f, 0);
			const int red = static_cast<int>(std::fmin(510.0f * static_cast<float>(v_max - value) / 100.0f, 255.0f));
			const int green = static_cast<int>(std::fmin(510.0f * static_cast<float>(value) / 100.0f, 255.0f));
			ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + ((h / float(v_max)) * (float)value)), ImColor(red, green, 25, 255), 0.0f, 0);
		}


		struct health_bar_state {
			float animated_health = 100.0f;
		};


		struct damage_flash_state {
			float last_health = 100.0f;
			float flash_alpha = 0.0f;
			float flash_timer = 0.0f;
		};


		ImU32 HSVtoRGB(float h, float s, float v, float a = 1.0f) {
			float c = v * s;
			float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
			float m = v - c;

			float r, g, b;
			if (h >= 0 && h < 60) {
				r = c; g = x; b = 0;
			} else if (h >= 60 && h < 120) {
				r = x; g = c; b = 0;
			} else if (h >= 120 && h < 180) {
				r = 0; g = c; b = x;
			} else if (h >= 180 && h < 240) {
				r = 0; g = x; b = c;
			} else if (h >= 240 && h < 300) {
				r = x; g = 0; b = c;
			} else {
				r = c; g = 0; b = x;
			}

			return IM_COL32((r + m) * 255, (g + m) * 255, (b + m) * 255, a * 255);
		}

		void HorizontalHealthBar(float x, float y, float w, float h, int value, int v_max, ImGuiID id = 0)
		{

			health_bar_state* state = nullptr;
			if (id != 0) {
				state = static_cast<health_bar_state*>(ImGui::GetStateStorage()->GetVoidPtr(id));
				if (!state) {
					state = new health_bar_state();
					state->animated_health = (float)value;
					ImGui::GetStateStorage()->SetVoidPtr(id, state);
				}


				float target_health = (float)value;
				float speed = (target_health > state->animated_health) ? 20.0f : 12.0f;
				state->animated_health = ImLerp(state->animated_health, target_health, ImGui::GetIO().DeltaTime * speed);
			}


			float display_health = (state && id != 0) ? state->animated_health : (float)value;
			float healthPercent = display_health / (float)v_max;
			float currentWidth = w * healthPercent;


			ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), ImColor(0, 0, 0, 180), 0.f, 0);


			ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), ImColor(40, 40, 40, 200), 0.0f, 0);


			if (currentWidth > 0.0f) {

				ImU32 healthColor;
				if (healthPercent > 0.75f) {
					healthColor = IM_COL32(50, 255, 60, 255);
				} else if (healthPercent > 0.44f) {
					healthColor = IM_COL32(255, 230, 50, 255);
				} else {
					healthColor = IM_COL32(255, 70, 50, 255);
				}

				ImGui::GetBackgroundDrawList()->AddRectFilled(
					ImVec2(x, y), ImVec2(x + currentWidth, y + h),
					healthColor, 0.0f
				);
			}


			if (h > 3.0f) {
				char hp_txt[16];
				snprintf(hp_txt, sizeof(hp_txt), "%d", (int)value);
				ImFont* f = ImGui::GetFont();
				float fsz = ImGui::GetFontSize();
				ImVec2 ts = f ? f->CalcTextSizeA(fsz, FLT_MAX, 0.0f, hp_txt) : ImVec2(0, 0);
				ImVec2 txt_center = ImVec2(x + w * 0.5f - ts.x * 0.5f, y + h * 0.5f - ts.y * 0.5f);
				ImGui::GetBackgroundDrawList()->AddText(txt_center, IM_COL32(255, 255, 255, 255), hp_txt);
			}
		}

		void CorneredBox(ImVec2 bottomCenter, float boxWidth, float pTop, float pBottom, ImColor color, float thickness, bool filled = true, float current_health = -1.0f, ImGuiID id = 0, float rounding = 4.0f)
		{

			damage_flash_state* flash_state = nullptr;
			if (current_health >= 0.0f && id != 0) {
				flash_state = static_cast<damage_flash_state*>(ImGui::GetStateStorage()->GetVoidPtr(id));
				if (!flash_state) {
					flash_state = new damage_flash_state();
					flash_state->last_health = current_health;
					ImGui::GetStateStorage()->SetVoidPtr(id, flash_state);
				}


				if (current_health < flash_state->last_health) {
					flash_state->flash_alpha = 80.0f;
					flash_state->flash_timer = 0.0f;
				}


				flash_state->last_health = current_health;


				if (flash_state->flash_alpha > 0.0f) {
					flash_state->flash_timer += ImGui::GetIO().DeltaTime;

					float flash_duration = 0.3f;
					float fade_progress = flash_state->flash_timer / flash_duration;

					if (fade_progress >= 1.0f) {
						flash_state->flash_alpha = 0.0f;
					} else {

						flash_state->flash_alpha = 80.0f * (1.0f - fade_progress);
					}
				}
			}

			float halfWidth = boxWidth / 2.0f;
			float boxHeight = (pBottom - pTop + 2.f);

			float cornerLengthX = boxHeight * 0.10f;
			float cornerLengthY = boxHeight * 0.10f;

			ImDrawList* draw = ImGui::GetForegroundDrawList();

			ImVec2 topLeft(bottomCenter.x - halfWidth, pTop);
			ImVec2 bottomRight(bottomCenter.x + halfWidth, pBottom);

			if (filled) {

				draw->AddRectFilled(
					topLeft, bottomRight,
					IM_COL32(
						0,
						0,
						0,
						50
					),
					rounding
				);


				if (flash_state && flash_state->flash_alpha > 0.0f) {
					draw->AddRectFilled(
						topLeft, bottomRight,
						IM_COL32(
							255,
							0,
							0,
							(int)flash_state->flash_alpha
						),
						rounding
					);
				}
			}


			const float PI = 3.14159265359f;


			ImVec2 tl_center = ImVec2(bottomCenter.x - halfWidth + rounding, pTop + rounding);
			draw->PathClear();
			draw->PathArcTo(tl_center, rounding, PI, PI * 1.5f, 8);
			draw->PathStroke(color, false, thickness);

			draw->AddLine(ImVec2(bottomCenter.x - halfWidth, pTop + rounding),
				ImVec2(bottomCenter.x - halfWidth, pTop + cornerLengthY), color, thickness);
			draw->AddLine(ImVec2(bottomCenter.x - halfWidth + rounding, pTop),
				ImVec2(bottomCenter.x - halfWidth + cornerLengthX, pTop), color, thickness);


			ImVec2 tr_center = ImVec2(bottomCenter.x + halfWidth - rounding, pTop + rounding);
			draw->PathClear();
			draw->PathArcTo(tr_center, rounding, PI * 1.5f, PI * 2.0f, 8);
			draw->PathStroke(color, false, thickness);

			draw->AddLine(ImVec2(bottomCenter.x + halfWidth, pTop + rounding),
				ImVec2(bottomCenter.x + halfWidth, pTop + cornerLengthY), color, thickness);
			draw->AddLine(ImVec2(bottomCenter.x + halfWidth - rounding, pTop),
				ImVec2(bottomCenter.x + halfWidth - cornerLengthX, pTop), color, thickness);


			ImVec2 bl_center = ImVec2(bottomCenter.x - halfWidth + rounding, pBottom - rounding);
			draw->PathClear();
			draw->PathArcTo(bl_center, rounding, PI * 0.5f, PI, 8);
			draw->PathStroke(color, false, thickness);

			draw->AddLine(ImVec2(bottomCenter.x - halfWidth, pBottom - rounding),
				ImVec2(bottomCenter.x - halfWidth, pBottom - cornerLengthY), color, thickness);
			draw->AddLine(ImVec2(bottomCenter.x - halfWidth + rounding, pBottom),
				ImVec2(bottomCenter.x - halfWidth + cornerLengthX, pBottom), color, thickness);


			ImVec2 br_center = ImVec2(bottomCenter.x + halfWidth - rounding, pBottom - rounding);
			draw->PathClear();
			draw->PathArcTo(br_center, rounding, 0.0f, PI * 0.5f, 8);
			draw->PathStroke(color, false, thickness);

			draw->AddLine(ImVec2(bottomCenter.x + halfWidth, pBottom - rounding),
				ImVec2(bottomCenter.x + halfWidth, pBottom - cornerLengthY), color, thickness);
			draw->AddLine(ImVec2(bottomCenter.x + halfWidth - rounding, pBottom),
				ImVec2(bottomCenter.x + halfWidth - cornerLengthX, pBottom), color, thickness);
		}

		void DrawCircle(ImVec2 center, float radius, const ImU32& color, int segments, float thickness)
		{
			ImGui::GetBackgroundDrawList()->AddCircle(center, radius, color, segments, thickness);
		}

		void DrawCircleFilled(ImVec2 center, float radius, const ImU32& color, int segments)
		{
			ImGui::GetBackgroundDrawList()->AddCircleFilled(center, radius, color, segments);
		}

		inline bool IsKeyPushing(int vKey) { return (GetAsyncKeyState(vKey) & 0x8000) != 0; }
		inline bool IsKeyPushed(int vKey) {
			static bool bSaveState[0x100] = { 0 };
			const bool bPrevState = bSaveState[(uint8_t)vKey];
			const bool bCurrState = IsKeyPushing(vKey);
			const bool Result = !bPrevState && bCurrState;
			bSaveState[(uint8_t)vKey] = bCurrState;
			return Result;
		}

	private:
		void create_target_view();
		void cleanup_target_view();

	private:
		HWND hWnd { nullptr };
		WNDCLASSW window_class { };
		ID3D11Device* device { nullptr };
		ID3D11DeviceContext* device_context { nullptr };
		IDXGISwapChain* swap_chain { nullptr };
    IDCompositionDevice* m_dcomp_device { nullptr };
    IDCompositionTarget* m_dcomp_target { nullptr };
    IDCompositionVisual* m_dcomp_visual { nullptr };
    bool m_use_dcomp { false };
		ID3D11RenderTargetView* render_target_view { nullptr };
		bool m_streamer_mode { false };





		bool m_watchdog_armed { false };
		uint64_t m_watchdog_tick { 0 };
		HWND m_last_hijack_hwnd { nullptr };



		char m_overlay_source[32] { 0 };
		uint64_t m_watchdog_last_lost_ms { 0 };
		void watchdog_tick();
		bool reacquire_target_window(bool discord_only = false);
		bool m_overlay_active { false };
		bool m_swapchain_owned { false };
		bool m_rendering_active { false };
	};

	inline std::unique_ptr<RENDER> m_render = std::make_unique<RENDER>();

}
