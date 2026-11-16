#pragma once

#include <string>



namespace maxi::discord_overlay {

    void set_frame_json(std::string json);

    void start_overlay_server(int port = 47777);


    void stop_overlay_server();


    bool install_discord_overlay_html();


    using SetCallback = void(*)(const std::string& key, bool value);
    void set_config_callback(SetCallback cb);


    bool overlay_html_active();

}
