#include <include/common.hpp>
#include <include/game/navigation.hpp>
#include <include/utils/logger.hpp>

namespace maxi::game::navigation {

    using namespace maxi::utils;

    NavigationManager::NavigationManager() {
    }

    std::filesystem::path NavigationManager::default_data_path() {


        wchar_t module_path[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(nullptr, module_path, MAX_PATH);
        if (length == 0 || length == MAX_PATH) {
            return std::filesystem::path("assets") / "navigation.json";
        }

        return std::filesystem::path(module_path).parent_path() / "assets" / "navigation.json";
    }

    bool NavigationManager::load_navigation() {
        return load_navigation(default_data_path());
    }

    bool NavigationManager::load_navigation(const std::filesystem::path& file) {
        try {
            std::ifstream stream(file, std::ios::binary | std::ios::ate);
            if (!stream) {
                LOG_ERROR(std::format("Failed to open navigation file: {}", file.string()));
                return false;
            }

            const std::streamoff size = stream.tellg();
            if (size < 0) {
                LOG_ERROR(std::format("Failed to determine navigation file size: {}", file.string()));
                return false;
            }

            std::string contents(static_cast<size_t>(size), '\0');
            stream.seekg(0, std::ios::beg);
            stream.read(contents.data(), static_cast<std::streamsize>(size));
            contents.resize(static_cast<size_t>(stream.gcount()));

            if (contents.empty()) {
                LOG_ERROR(std::format("Lineups file is empty: {}", file.string()));
                return false;
            }

            nlohmann::json json_data = nlohmann::json::parse(contents);

            if (!json_data.contains("Lineups") || !json_data["Lineups"].is_array()) {
                LOG_ERROR("Invalid JSON format: missing 'Lineups' array");
                return false;
            }

            const auto& entries = json_data["Lineups"];

            m_navigation.clear();
            m_navigation.reserve(entries.size());
            size_t total_count = 0;

            for (const auto& nav_json : entries) {
                try {
                    NavPoint navpoint;
                    navpoint.agent = nav_json.value("Agent", "");
                    navpoint.distance = nav_json.value("Distance", 0.0);
                    navpoint.map = nav_json.value("Map", "");
                    navpoint.position_x = nav_json.value("PositionX", 0.0);
                    navpoint.position_y = nav_json.value("PositionY", 0.0);
                    navpoint.position_z = nav_json.value("PositionZ", 0.0);
                    navpoint.rotation_x = nav_json.value("RotationX", 0.0);
                    navpoint.rotation_y = nav_json.value("RotationY", 0.0);
                    navpoint.rotation_z = nav_json.value("RotationZ", 0.0);
                    navpoint.tips = nav_json.value("Tips", "");
                    navpoint.weapon = nav_json.value("Weapon", "");
                    navpoint.timing = nav_json.value("Timing", 0.0);

                    m_navigation.push_back(navpoint);
                    total_count++;
                }
                catch (const std::exception& e) {
                    LOG_ERROR(std::format("Error parsing navpoint entry: {}", e.what()));
                    continue;
                }
            }

            LOG_INFO(std::format("Loaded {} navigation from {}", total_count, file.string()));
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR(std::format("Exception loading navigation: {}", e.what()));
            return false;
        }
    }

    std::vector<NavPoint> NavigationManager::get_navigation(const std::string& agent_name, const std::string& map_name, const std::string& weapon_name) const {
        std::vector<NavPoint> filtered_navigation;

        for (const auto& navpoint : m_navigation) {
            if (navpoint.agent == agent_name &&
                navpoint.map == map_name &&
                navpoint.weapon == weapon_name) {
                filtered_navigation.push_back(navpoint);
            }
        }

        return filtered_navigation;
    }

    std::optional<NavPoint> NavigationManager::get_nearest_navpoint(
        const maxi::game::model::FVector& current_position,
        const std::string& agent_name,
        const std::string& map_name,
        const std::string& weapon_name,
        double max_distance
    ) const {
        std::optional<NavPoint> nearest_navpoint;
        double min_distance = max_distance;

        for (const auto& navpoint : m_navigation) {
            if (navpoint.agent == agent_name &&
                navpoint.map == map_name &&
                navpoint.weapon == weapon_name) {

                maxi::game::model::FVector nav_position{
                    static_cast<float>(navpoint.position_x),
                    static_cast<float>(navpoint.position_y),
                    static_cast<float>(navpoint.position_z)
                };

                double distance = calculate_distance(current_position, nav_position);

                if (distance < min_distance) {
                    min_distance = distance;
                    nearest_navpoint = navpoint;
                }
            }
        }

        return nearest_navpoint;
    }

    size_t NavigationManager::get_navigation_count(const std::string& agent_name, const std::string& map_name, const std::string& weapon_name) const {
        size_t count = 0;

        for (const auto& navpoint : m_navigation) {
            if (navpoint.agent == agent_name &&
                navpoint.map == map_name &&
                navpoint.weapon == weapon_name) {
                count++;
            }
        }

        return count;
    }

    double NavigationManager::calculate_distance(const maxi::game::model::FVector& pos1, const maxi::game::model::FVector& pos2) const {
        double dx = pos1.X - pos2.X;
        double dy = pos1.Y - pos2.Y;
        double dz = pos1.Z - pos2.Z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }


    std::unique_ptr<NavigationManager> g_navigation = std::make_unique<NavigationManager>();

}