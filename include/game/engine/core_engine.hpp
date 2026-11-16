#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>

#include <include/game/kernel_protocol.hpp>
#include <include/game/model/ue_object.hpp>
#include <include/game/model/uworld.hpp>
#include <include/game/model/ulevel.hpp>
#include <include/game/model/ugame_instance.hpp>
#include <include/game/model/ulocalplayer.hpp>
#include <include/game/model/aplayer_camera_manager.hpp>
#include <include/game/model/aplayer_controller.hpp>
#include <include/game/model/aactor.hpp>
#include <include/driver/idriver.hpp>
#include <include/utils/logger.hpp>

namespace maxi::game::engine {

    struct CameraCache {
        model::FVector LastValidLocation{};
        model::FRotator LastValidRotation{};
        float LastValidFOV = 90.0f;
        bool HasValidCache = false;
        std::chrono::steady_clock::time_point timestamp{};
    };

    inline CameraCache m_cameraCache;
    inline std::mutex m_cameraCacheMutex;

    inline std::atomic<uintptr_t> m_local_pawn_addr{ 0 };

    


    struct WorldData {
        model::UWorld world{0};
        model::ULevel level{ 0 };
        model::UGameInstance game_instance{ 0 };
		model::TArray<model::AActor> actors{};
        model::ULocalPlayer local_player{ 0 };
        model::APlayerController player_controller{ 0 };
		model::AActor acknowledged_pawn{ 0 };
        model::APlayerCameraManager player_camera_manager{ 0 };
        std::string local_player_name;
        bool is_valid = false;
        std::chrono::steady_clock::time_point last_update;
    };

    struct ActorInfo {
		model::AActor                                     actor;
        std::uintptr_t                                  mesh;
        std::uintptr_t                                  bone_array;
        std::string                                     name;
        std::string                                     weapon_name;
        model::ADamageHandler                             damage_handler;
        bool                                            visible;
        int32_t                                         team_id = -1;
        std::array<model::FVector, 17>                  world_bones{};
        float                                           health = 0.f;

        bool                                            is_teammate = false;
        bool                                            aux_resolved = false;

        bool                                            hidden = false;


        bool                                            is_dormant = false;

        float                                           facing_yaw = 0.f;
        bool                                            has_facing = false;

        uint8_t                                         dead_streak = 0;

        model::FVector                                  bone_vel{};
        uint64_t                                        bone_ms = 0;
        bool                                            have_vel = false;
        float                                           bone_interval = 50.f;
        bool                                            bones_synth = false;
        uint64_t                                        health_ms = 0;
    };

    struct SkillInfo {
        model::UObject    skill;
		std::string     name;
    };

    


    struct ActorsData {
        std::vector<ActorInfo> players_list;
        std::chrono::steady_clock::time_point last_update;
        int32_t local_team_id = -1;
    };

    struct SkillsData {
        std::vector<SkillInfo> skills_list;
        model::UObject timed_bomb{ 0 };
        std::chrono::steady_clock::time_point last_update;
    };

    






    class CoreEngine {
    public:
        CoreEngine();
        ~CoreEngine();


        bool initialize();
        void shutdown();


        void start();
        void stop();
        void pause();
        void resume();
        bool is_running() const { return m_running.load(); }
        bool is_paused() const { return m_paused.load(); }


        bool set_target_process(const std::wstring& process_name);
        bool set_target_process(uint32_t process_id);


        WorldData get_world_data() const;
        ActorsData get_actors_data() const;
        SkillsData get_skills_data() const;


        void set_world_update_callback(std::function<void(const WorldData&)> callback);
        void set_actors_update_callback(std::function<void(const ActorsData&)> callback);

        std::optional<std::pair<std::array<model::FVector2D, 17>, std::array<std::array<model::FVector2D, 2>, 16>>> SetupActorSkeleton(model::AActor actor, uintptr_t mesh, uintptr_t bone_array);

        std::optional<std::array<model::FVector, 17>> GetActorBoneWorldPositions(model::AActor actor, uintptr_t mesh, uintptr_t bone_array);


        std::shared_ptr<KernelProtocol> get_kernel_protocol() const { return m_kernel_protocol; }

        model::FVector2D ProjectWorldToScreen(model::FVector world_location);



        model::FMinimalViewInfo get_cached_camera_view() const {
            model::FMinimalViewInfo v{};
            std::lock_guard<std::mutex> lock(m_cameraCacheMutex);
            if (m_cameraCache.HasValidCache) {
                v.Location = m_cameraCache.LastValidLocation;
                v.Rotation = m_cameraCache.LastValidRotation;
                v.FOV = m_cameraCache.LastValidFOV;
            }
            return v;
        }

        uint64_t camera_age_ms() const {
            std::lock_guard<std::mutex> lock(m_cameraCacheMutex);
            if (!m_cameraCache.HasValidCache) return 99999;
            return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_cameraCache.timestamp).count();
        }

        void refresh_camera_cache(const model::APlayerCameraManager& pcm);

        void set_screen_dimensions(float width, float height) {
            if (width > 0.0f && height > 0.0f) {
                m_screen_width = width;
                m_screen_height = height;
            }
        }

        bool inScreen(const maxi::game::model::FVector2D& bone) {
            return bone.isValid() &&
                bone.X > 0 && bone.X < m_screen_width &&
                bone.Y > 0 && bone.Y < m_screen_height;
        }

    private:


        std::atomic<bool> m_running{false};
        std::atomic<bool> m_paused{false};
        std::unique_ptr<std::thread> m_world_thread;
        std::unique_ptr<std::thread> m_camera_thread;
        std::unique_ptr<std::thread> m_actors_thread;
		std::unique_ptr<std::thread> m_skills_thread;
        std::unique_ptr<std::thread> m_bones_thread;


        mutable std::mutex m_world_data_mutex;
        mutable std::mutex m_actors_data_mutex;
		mutable std::mutex m_skills_data_mutex;
        WorldData m_world_data;
        ActorsData m_actors_data;
        SkillsData m_skills_data;


        std::function<void(const WorldData&)> m_world_callback;
        std::function<void(const ActorsData&)> m_actors_callback;


        void world_thread_loop();
        void camera_thread_loop();
        void actors_thread_loop();
		void skills_thread_loop();
        void bones_thread_loop();


        uintptr_t resolve_world_address();
        uintptr_t resolve_world_address_uncached();
        void update_world_data();
        void update_actors_data();
        void update_skills_data();
        bool is_world_data_valid(const WorldData& data) const;

		std::string GetAgentName(std::string name) const;
		std::string GetSkillName(std::string name) const;

		static std::string MapWeaponName(const std::string& raw);


        std::atomic<bool> m_needs_refresh{false};
        std::atomic<uintptr_t> m_uworld_offset{0};


        uintptr_t m_last_world_addr{0};

        float m_screen_width  = 1920.0f;
        float m_screen_height = 1080.0f;
    };

	inline std::shared_ptr<CoreEngine> m_core_engine = std::make_shared<CoreEngine>();

}
