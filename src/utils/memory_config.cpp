#include "../../include/utils/config.hpp"
#include <algorithm>
#include <sstream>

namespace maxi::utils {

    


    class MemoryConfigManager : public IConfigManager {
    public:
        MemoryConfigManager() = default;

        void set_value(const std::string& key, const ConfigValue& value) override {
            std::string full_key = get_full_key(key);
            m_config[full_key] = value;
        }

        ConfigValue get_value(const std::string& key, const ConfigValue& default_value) const override {
            std::string full_key = get_full_key(key);
            auto it = m_config.find(full_key);
            if (it != m_config.end()) {
                return it->second;
            }
            return default_value;
        }

        bool has_value(const std::string& key) const override {
            std::string full_key = get_full_key(key);
            return m_config.find(full_key) != m_config.end();
        }

        void remove_value(const std::string& key) override {
            std::string full_key = get_full_key(key);
            m_config.erase(full_key);
        }


        std::string get_string(const std::string& key, const std::string& default_value) const override {
            auto val = get_value(key, ConfigValue());
            if (val.type == ConfigType::STRING && std::holds_alternative<std::string>(val.value)) {
                return std::get<std::string>(val.value);
            }
            return default_value;
        }

        int get_int(const std::string& key, int default_value) const override {
            auto val = get_value(key, ConfigValue());
            if (val.type == ConfigType::INT && std::holds_alternative<int>(val.value)) {
                return std::get<int>(val.value);
            }
            return default_value;
        }

        double get_double(const std::string& key, double default_value) const override {
            auto val = get_value(key, ConfigValue());
            if (val.type == ConfigType::DOUBLE && std::holds_alternative<double>(val.value)) {
                return std::get<double>(val.value);
            }
            return default_value;
        }

        bool get_bool(const std::string& key, bool default_value) const override {
            auto val = get_value(key, ConfigValue());
            if (val.type == ConfigType::BOOL && std::holds_alternative<bool>(val.value)) {
                return std::get<bool>(val.value);
            }
            return default_value;
        }


        bool load_from_file(const std::string& filename) override {

            return false;
        }

        bool save_to_file(const std::string& filename) const override {

            return false;
        }

        bool load_from_json(const std::string& json_content) override {

            return false;
        }

        std::string save_to_json() const override {

            return "{}";
        }


        void set_section(const std::string& section) override {
            m_current_section = section;
        }

        std::string get_section() const override {
            return m_current_section;
        }

        void clear_section() override {
            m_current_section.clear();
        }

    private:
        std::map<std::string, ConfigValue> m_config;
        std::string m_current_section;

        std::string get_full_key(const std::string& key) const {
            if (m_current_section.empty()) {
                return key;
            }
            return m_current_section + "." + key;
        }
    };


    std::shared_ptr<IConfigManager> ConfigManagerFactory::create_memory_config() {
        return std::make_shared<MemoryConfigManager>();
    }

    std::shared_ptr<IConfigManager> ConfigManagerFactory::create_file_config(const std::string& filename) {

        return create_memory_config();
    }

    std::shared_ptr<IConfigManager> ConfigManagerFactory::create_json_config(const std::string& filename) {

        return create_memory_config();
    }


    std::shared_ptr<IConfigManager> g_config = ConfigManagerFactory::create_memory_config();

}