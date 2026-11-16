#pragma once

#include <include/common.hpp>
#include <include/game/model/ue_object.hpp>
#include <include/game/model/aactor.hpp>
#include <include/game/engine/core_engine.hpp>
#include <include/driver/driver_context.hpp>
#include <include/render/settings/variables.h>
#include <algorithm>
#include <cmath>

namespace maxi::game::combat {

	using namespace maxi::game::engine;
	using namespace maxi::game::model;

	struct BoneMap {
		int head, neck, chest;
		int lUpperArm, lForeArm, lHand;
		int rUpperArm, rForeArm, rHand;
		int stomach, pelvis;
		int lThigh, lKnee, lFoot;
		int rThigh, rKnee, rFoot;
	};

	inline std::unordered_map<int, BoneMap> boneMaps = {
		{ 104, {8,21,6, 23,24,25, 49,50,51, 4,3, 77,78,80, 84,85,87} },
		{ 101, {8,21,6, 23,24,25, 49,50,51, 4,3, 75,76,78, 82,83,85} },
		{ 103, {8,9,6, 33,30,32, 58,55,57, 4,3, 63,65,69, 77,79,83} }
	};

	


class ICombat {
private:
		model::AActor m_target;
		std::uintptr_t bone_array = 0;
		float m_best_dist = 999999.0f;
		float m_screen_width = 1920.0f;
		float m_screen_height = 1080.0f;


		std::mutex m_mutex;
		std::thread m_thread;
		std::atomic<bool> m_running{ false };


		double m_residual_x = 0.0;
		double m_residual_y = 0.0;

		static float Distance2D(float x1, float y1, float x2, float y2) {
			float dx = x1 - x2;
			float dy = y1 - y2;
			return std::sqrt(dx * dx + dy * dy);
		}

		model::FRotator CalculateAngles(const model::FVector& src, const model::FVector& dst) {
			model::FVector delta = dst - src;
			double hyp = std::sqrt(delta.X * delta.X + delta.Y * delta.Y);
			if (hyp < 0.0001) {
				return model::FRotator(0.0, 0.0, 0.0);
			}

			model::FRotator angles;

			angles.Pitch = std::atan2(delta.Z, hyp) * model::Constants::RAD_TO_DEG;

			angles.Yaw = std::atan2(delta.Y, delta.X) * model::Constants::RAD_TO_DEG;
			angles.Roll = 0.0;

			angles.Normalize();
			return angles;
		}

	public:
		ICombat() = default;
		~ICombat() {
			stop_thread();
		}

		void start_thread() {

			return;
		}

		void stop_thread() {
			if (m_running.load()) {
				m_running.store(false);
				if (m_thread.joinable()) {
					m_thread.join();
				}
			}
		}

		void set_screen_dimensions(float width, float height) {
			std::lock_guard<std::mutex> lock(m_mutex);
			if (width > 0.0f && height > 0.0f) {
				m_screen_width = width;
				m_screen_height = height;
			}
		}

		void reset_target() {
			std::lock_guard<std::mutex> lock(m_mutex);
			m_target = model::AActor();
			bone_array = 0;
			m_best_dist = 999999.0f;
			m_residual_x = 0.0;
			m_residual_y = 0.0;
		}

		void begin_frame() {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (!var->config.combat.sticky_target || !m_target.is_valid()) {
				m_target = model::AActor();
				bone_array = 0;
				m_best_dist = 999999.0f;
			}
		}

		float DistanceBetweenCross(float x, float y) const {
			float center_x = m_screen_width * 0.5f;
			float center_y = m_screen_height * 0.5f;
			return Distance2D(x, y, center_x, center_y);
		}

		void try_select_target(model::AActor candidate, const std::array<model::FVector2D, 17>& bones, bool is_visible = true) {
			(void)candidate; (void)bones; (void)is_visible;
			return;
		}

		void run_step() {
			return;
/* disabled original:
			if (!var->config.combat.enabled) {
				reset_target();
				return;
			}


			bool key_pressed = false;
			if (var->config.combat.key != 0) {
				key_pressed = (GetAsyncKeyState(var->config.combat.key) & 0x8000) != 0;
			}

			if (!key_pressed) {
				reset_target();
				return;
			}

			model::AActor current_target;
			std::uintptr_t current_bone_array = 0;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				current_target = m_target;
				current_bone_array = bone_array;
			}

			if (!current_target.is_valid() || current_bone_array == 0) {
				reset_target();
				return;
			}

			auto damage_handler = current_target.DamageHandler();
			if (damage_handler.GetHealth() <= 0.0f) {
				reset_target();
				return;
			}

			if (var->config.combat.visible_only && var->config.combat.game_mode == 0) {
				if (!current_target.IsVisible(current_target.Mesh())) {
					reset_target();
					return;
				}
			}

			auto bone_count = current_target.BoneCount(current_bone_array);
			if (bone_count <= 0) {
				reset_target();
				return;
			}

			auto it = boneMaps.find(bone_count);
			if (it == boneMaps.end()) {
				reset_target();
				return;
			}
			const BoneMap& map = it->second;

			model::FVector selected_bone_world;
			if (var->config.combat.mode == 0) {

				int target_bone_id = map.chest;
				switch (var->config.combat.bone) {
					case 0: target_bone_id = map.head; break;
					case 1: target_bone_id = map.neck; break;
					case 2: target_bone_id = map.chest; break;
					case 3: target_bone_id = map.pelvis; break;
					default: target_bone_id = map.chest; break;
				}
				selected_bone_world = current_target.GetBoneWithRotation(current_target.Mesh(), current_bone_array, target_bone_id);
			} else {

				float min_cross_dist = std::numeric_limits<float>::max();
				const int candidate_bones[] = {
					map.head, map.neck, map.chest,
					map.lUpperArm, map.lForeArm, map.lHand,
					map.rUpperArm, map.rForeArm, map.rHand,
					map.stomach, map.pelvis,
					map.lThigh, map.lKnee, map.lFoot,
					map.rThigh, map.rKnee, map.rFoot
				};

				for (int bone_idx : candidate_bones) {
					model::FVector bone_pos = current_target.GetBoneWithRotation(current_target.Mesh(), current_bone_array, bone_idx);
					auto bone_screen = m_core_engine->ProjectWorldToScreen(bone_pos);
					if (bone_screen.isValid()) {
						float dist = DistanceBetweenCross(static_cast<float>(bone_screen.X), static_cast<float>(bone_screen.Y));
						if (dist < min_cross_dist) {
							min_cross_dist = dist;
							selected_bone_world = bone_pos;
						}
					}
				}
			}

			if (selected_bone_world.X == 0.0 && selected_bone_world.Y == 0.0 && selected_bone_world.Z == 0.0) {
				reset_target();
				return;
			}

			auto w2s = m_core_engine->ProjectWorldToScreen(selected_bone_world);
			if (!w2s.isValid()) {
				reset_target();
				return;
			}

			float dist_to_cross = DistanceBetweenCross(static_cast<float>(w2s.X), static_cast<float>(w2s.Y));
			if (dist_to_cross > var->config.combat.fov) {
				reset_target();
				return;
			}


			float deadzone = var->config.combat.fov * 0.2f;
			if (dist_to_cross <= deadzone && deadzone > 0.0f) {
				return;
			}

			if (!m_cameraCache.HasValidCache) {
				reset_target();
				return;
			}

			model::FRotator current_rotation = m_cameraCache.LastValidRotation;
			auto world_data = m_core_engine->get_world_data();
			if (world_data.player_controller.is_valid()) {
				current_rotation = world_data.player_controller.ControlRotation();
			}

			model::FRotator aim_rotation = CalculateAngles(m_cameraCache.LastValidLocation, selected_bone_world);
			model::FRotator delta_rotation;

			if (var->config.combat.rcs_enabled) {
				model::FRotator recoil_delta = m_cameraCache.LastValidRotation - current_rotation;
				recoil_delta.Normalize();

				double strength_factor = static_cast<double>(var->config.combat.rcs_strength) / 100.0;
				recoil_delta.Pitch *= strength_factor;
				recoil_delta.Yaw *= strength_factor;

				model::FRotator compensated_rotation = aim_rotation - recoil_delta;
				compensated_rotation.Normalize();

				delta_rotation = compensated_rotation - current_rotation;
				delta_rotation.Normalize();
			} else {
				delta_rotation = aim_rotation - current_rotation;
				delta_rotation.Normalize();
			}


			double smooth_val = static_cast<double>(var->config.combat.smooth);
			if (smooth_val < 1.0) {
				smooth_val = 1.0;
			}


			if (var->config.combat.humanization_bezier > 0.0f) {
				double curve_factor = 1.0 + (static_cast<double>(var->config.combat.humanization_bezier) * 0.1) * (dist_to_cross / (var->config.combat.fov + 0.001f));
				smooth_val *= curve_factor;
			}

			delta_rotation.Yaw /= smooth_val;
			delta_rotation.Pitch /= smooth_val;



			double sens = static_cast<double>(var->config.combat.sensitivity);
			if (sens <= 0.001) {
				sens = 1.0;
			}

			const double angle_per_pixel = 0.022 * (sens * 3.125 / 3.125);
			double target_dx = (delta_rotation.Yaw / (0.022 * sens)) + m_residual_x;
			double target_dy = (-delta_rotation.Pitch / (0.022 * sens)) + m_residual_y;

			int32_t move_x = static_cast<int32_t>(std::round(target_dx));
			int32_t move_y = static_cast<int32_t>(std::round(target_dy));

			m_residual_x = target_dx - static_cast<double>(move_x);
			m_residual_y = target_dy - static_cast<double>(move_y);

			// [READ-ONLY] move_mouse call removed.
			(void)move_x; (void)move_y;
			return;
/*
			if (move_x != 0 || move_y != 0) {
				maxi::driver::g_driver->move_mouse(move_x, move_y, 0);
			}
*/
		}
	};
/* run_step end wrapped */

	inline std::shared_ptr<ICombat> m_combat = std::make_shared<ICombat>();
}
