#pragma once

#include "ue_object.hpp"
#include "ulocalplayer.hpp"
#include <vector>
#include <optional>

namespace maxi::game::model {

    


    class UGameInstance : public UObject {
    public:
        UGameInstance() : UObject() {}


        explicit UGameInstance(uintptr_t base_address)
            : UObject(base_address) {}


        ULocalPlayer get_local_player() const {
            if (!is_valid()) return {};


            for (uintptr_t off : memory_map::LocalPlayersCandidates) {
                uintptr_t tarray_data = read<uintptr_t>(off);
                if (!tarray_data || tarray_data < 0x10000) continue;


                uintptr_t local_player_ptr = 0;
                if (maxi::driver::g_driver)
                    maxi::driver::g_driver->read_memory(tarray_data, &local_player_ptr, sizeof(local_player_ptr));

                if (local_player_ptr > 0x10000000000ull && local_player_ptr < 0x40000000000ull) {
                    ULocalPlayer lp(local_player_ptr);
                    uintptr_t pc = lp.get_player_controller().get_base_address();
                    if (pc > 0x10000000000ull && pc < 0x40000000000ull)
                        return lp;
                }


                ULocalPlayer direct(tarray_data);
                uintptr_t pc2 = direct.get_player_controller().get_base_address();
                if (pc2 > 0x10000000000ull && pc2 < 0x40000000000ull)
                    return direct;
            }

            return {};
        }

        std::optional<ULocalPlayer> TryGetLocalPlayer() const {
            if (!is_valid()) return std::nullopt;

            for (uintptr_t off : memory_map::LocalPlayersCandidates) {
                auto tarray_data_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + off);
                if (!tarray_data_opt) continue;
                uintptr_t tarray_data = *tarray_data_opt;
                if (!tarray_data || tarray_data < 0x10000) continue;

                auto local_player_ptr_opt = maxi::driver::g_driver->try_read<uintptr_t>(tarray_data);
                if (local_player_ptr_opt) {
                    uintptr_t local_player_ptr = *local_player_ptr_opt;
                    if (local_player_ptr > 0x10000000000ull && local_player_ptr < 0x40000000000ull) {
                        ULocalPlayer lp(local_player_ptr);
                        auto pc_opt = lp.TryGetPlayerController();
                        if (pc_opt && pc_opt->get_base_address() > 0x10000000000ull && pc_opt->get_base_address() < 0x40000000000ull)
                            return lp;
                    }
                }


                ULocalPlayer direct(tarray_data);
                auto pc2_opt = direct.TryGetPlayerController();
                if (pc2_opt && pc2_opt->get_base_address() > 0x10000000000ull && pc2_opt->get_base_address() < 0x40000000000ull)
                    return direct;
            }

            return std::nullopt;
        }

        [[deprecated("Use TryGetLocalPlayer()")]]
        ULocalPlayer get_local_player_compat() const { return get_local_player(); }


        void print_debug() const;
    };

}

