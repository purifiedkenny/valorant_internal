#pragma once

#include <include/common.hpp>
#include <include/game/model/model.hpp>

#include <filesystem>

namespace maxi::game::navigation {

    


    struct NavPoint {
        std::string agent;
        double distance;
        std::string map;
        double position_x;
        double position_y;
        double position_z;


        double rotation_x;
        double rotation_y;
        double rotation_z;
        std::string tips;
        std::string weapon;
        double timing = 0.0;
    };

    


    class NavigationManager {
    public:
        NavigationManager();
        ~NavigationManager() = default;

        







        bool load_navigation();

        




        bool load_navigation(const std::filesystem::path& file);

        


        static std::filesystem::path default_data_path();

        






        std::vector<NavPoint> get_navigation(const std::string& agent_name, const std::string& map_name, const std::string& weapon_name) const;

        








        std::optional<NavPoint> get_nearest_navpoint(
            const maxi::game::model::FVector& current_position,
            const std::string& agent_name,
            const std::string& map_name,
            const std::string& weapon_name,
            double max_distance = 100.0
        ) const;

        



        bool has_navigation() const { return !m_navigation.empty(); }

        



        size_t get_navigation_count() const { return m_navigation.size(); }

        






        size_t get_navigation_count(const std::string& agent_name, const std::string& map_name, const std::string& weapon_name) const;

    private:
        std::vector<NavPoint> m_navigation;

        





        double calculate_distance(const maxi::game::model::FVector& pos1, const maxi::game::model::FVector& pos2) const;
    };


    extern std::unique_ptr<NavigationManager> g_navigation;

}
