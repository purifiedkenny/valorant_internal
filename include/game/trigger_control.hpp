#include <include/common.hpp>
#include <include/game/model/ue_object.hpp>
#include <include/game/model/aactor.hpp>
#include <include/game/engine/core_engine.hpp>
#include <include/render/settings/variables.h>

namespace maxi::game::trigger_control {

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

	


	class ITriggerControl {
	private:
		std::chrono::steady_clock::time_point last_shot_time;

		float m_screen_width = 1920.0f;
		float m_screen_height = 1080.0f;

		float DistanceBetweenCross(float X, float Y)
		{
			float xdist = X - (float)(m_screen_width / 2.0f);
			float ydist = Y - (float)(m_screen_height / 2.0f);


			return sqrtf((xdist * xdist) + (ydist * ydist));
		}


		float DistanceSquaredBetweenCross(float X, float Y)
		{
			float xdist = X - (float)(m_screen_width / 2.0f);
			float ydist = Y - (float)(m_screen_height / 2.0f);


			return (xdist * xdist) + (ydist * ydist);
		}

		bool CanShoot() {
			auto now = std::chrono::steady_clock::now();
			auto time_since_last_shot = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_shot_time);
			return time_since_last_shot.count() >= var->config.trigger_control.delay_shot;
		}


		float DistanceToLine(float px, float py, float x1, float y1, float x2, float y2) {
			float A = px - x1;
			float B = py - y1;
			float C = x2 - x1;
			float D = y2 - y1;

			float dot = A * C + B * D;
			float len_sq = C * C + D * D;

			if (len_sq == 0) {

				return sqrtf(A * A + B * B);
			}

			float param = dot / len_sq;

			float xx, yy;
			if (param < 0) {
				xx = x1;
				yy = y1;
			} else if (param > 1) {
				xx = x2;
				yy = y2;
			} else {
				xx = x1 + param * C;
				yy = y1 + param * D;
			}

			float dx = px - xx;
			float dy = py - yy;
			return sqrtf(dx * dx + dy * dy);
		}


		bool IsNearSkeleton(const std::array<std::array<model::FVector2D, 2>, 16>& skeleton, float threshold) {
			float crossX = (float)(m_screen_width / 2.0f);
			float crossY = (float)(m_screen_height / 2.0f);

			for (const auto& line : skeleton) {
				float dist = DistanceToLine(
					crossX,
					crossY,
					static_cast<float>(line[0].X),
					static_cast<float>(line[0].Y),
					static_cast<float>(line[1].X),
					static_cast<float>(line[1].Y));
				if (dist <= threshold) {
					return true;
				}
			}
			return false;
		}

	public:
		ITriggerControl() = default;
		~ITriggerControl() = default;

		void set_screen_dimensions(float width, float height) {
			if (width > 0.0f && height > 0.0f) {
				m_screen_width = width;
				m_screen_height = height;
			}
		}

		void run(model::AActor actor, std::array<model::FVector2D, 17> bones, std::array<std::array<model::FVector2D, 2>, 16> skeleton) {
			(void)actor; (void)bones; (void)skeleton;
			return;
		}
	};

	inline std::shared_ptr<ITriggerControl> m_trigger_control = std::make_shared<ITriggerControl>();
}
