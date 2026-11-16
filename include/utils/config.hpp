#pragma once

#include <string>
#include <map>
#include <memory>
#include <variant>

namespace maxi::utils {

    


    enum class ConfigType {
        STRING,
        INT,
        DOUBLE,
        BOOL
    };

    


    struct ConfigValue {
        ConfigType type;
        std::variant<std::string, int, double, bool> value;

        ConfigValue() : type(ConfigType::STRING), value(std::string("")) {}

        ConfigValue(const std::string& val) : type(ConfigType::STRING), value(val) {}

        ConfigValue(int val) : type(ConfigType::INT), value(val) {}

        ConfigValue(double val) : type(ConfigType::DOUBLE), value(val) {}

        ConfigValue(bool val) : type(ConfigType::BOOL), value(val) {}
    };

    


    class IConfigManager {
    public:
        virtual ~IConfigManager() = default;


        virtual void set_value(const std::string& key, const ConfigValue& value) = 0;
        virtual ConfigValue get_value(const std::string& key, const ConfigValue& default_value = ConfigValue()) const = 0;
        virtual bool has_value(const std::string& key) const = 0;
        virtual void remove_value(const std::string& key) = 0;


        virtual std::string get_string(const std::string& key, const std::string& default_value = "") const = 0;
        virtual int get_int(const std::string& key, int default_value = 0) const = 0;
        virtual double get_double(const std::string& key, double default_value = 0.0) const = 0;
        virtual bool get_bool(const std::string& key, bool default_value = false) const = 0;


        virtual bool load_from_file(const std::string& filename) = 0;
        virtual bool save_to_file(const std::string& filename) const = 0;
        virtual bool load_from_json(const std::string& json_content) = 0;
        virtual std::string save_to_json() const = 0;


        virtual void set_section(const std::string& section) = 0;
        virtual std::string get_section() const = 0;
        virtual void clear_section() = 0;
    };

    


    class ConfigManagerFactory {
    public:
        static std::shared_ptr<IConfigManager> create_memory_config();
        static std::shared_ptr<IConfigManager> create_file_config(const std::string& filename);
        static std::shared_ptr<IConfigManager> create_json_config(const std::string& filename);
    };


    extern std::shared_ptr<IConfigManager> g_config;

}
