#pragma once

#include <memory>
#include <string>
#include "../driver/idriver.hpp"
#include "../game/kernel_protocol.hpp"

namespace maxi::core {
    





    class IApplication {
    public:
        virtual ~IApplication() = default;


        virtual bool initialize() = 0;
        virtual void run() = 0;
        virtual void shutdown() = 0;
        virtual bool is_running() const = 0;


        virtual void set_target_process(const std::wstring& process_name) = 0;
        virtual void set_configuration(const std::string& config_name, const std::string& value) = 0;


        virtual void pause() = 0;
        virtual void resume() = 0;
        virtual void stop() = 0;

        virtual void full_unload() = 0;
    };

    inline std::shared_ptr<IApplication> g_app;


    inline void request_full_unload() {
        if (g_app) g_app->full_unload();
    }

    std::shared_ptr<IApplication> create_application();

}
