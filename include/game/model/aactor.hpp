#pragma once

#include <vector>
#include <algorithm>
#include <optional>
#include "ue_object.hpp"
#include "aplayer_state.hpp"
#include "adamage_handler.hpp"
#include "../../driver/backend.h"

namespace maxi::game::model {

    
    
    
    
    class UVisibilityComponent : public UObject {
    public:
        UVisibilityComponent() : UObject() {}
        explicit UVisibilityComponent(uintptr_t base_address) : UObject(base_address) {}

        std::optional<float> TryVisibilityLostTime() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<float>(m_base_address + memory_map::VisibilityLostTime);
        }

        [[deprecated("Use TryVisibilityLostTime()")]]
        float VisibilityLostTime() const { return TryVisibilityLostTime().value_or(0.0f); }



        std::optional<bool> TryPerfectVisCheck(uintptr_t local_player_addr) const {
            if (!is_valid() || !local_player_addr) return std::nullopt;

            auto arr_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::CharactersWithVisibilityOffset);
            if (!arr_opt || !*arr_opt) return false;

            uintptr_t entries[10] = {};
            if (!maxi::driver::g_driver->read_memory(*arr_opt, entries, sizeof(entries))) return false;
            for (int i = 0; i < 10; ++i) {
                if (entries[i] == local_player_addr) return true;
            }
            return false;
        }
    };


    
    
    
    class AShooterCharacterMinimapComponent : public UObject {
    public:
        AShooterCharacterMinimapComponent() : UObject() {}
        explicit AShooterCharacterMinimapComponent(uintptr_t base_address) : UObject(base_address) {}

        std::optional<bool> TryBVisible() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<bool>(m_base_address + memory_map::MinimapVisible);
		}

        [[deprecated("Use TryBVisible()")]]
        bool bVisible() const { return TryBVisible().value_or(false); }

        std::optional<UVisibilityComponent> TryVisibilityComponent() const {
            if (!is_valid()) return std::nullopt;

            auto vc_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::VisibilityComponentOffset);
            if (!vc_opt || !*vc_opt) return std::nullopt;
            return UVisibilityComponent(*vc_opt);
        }

        [[deprecated("Use TryVisibilityComponent()")]]
        UVisibilityComponent VisibilityComponent() const { return TryVisibilityComponent().value_or(UVisibilityComponent{}); }
    };
    
    
    
    
    class AActor : public UObject {
    public:
        AActor() : UObject() {}
        explicit AActor(uintptr_t base_address) : UObject(base_address) {}

        std::optional<int32_t> TryUniqueID() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<int32_t>(m_base_address + memory_map::UniqueID);
        }

        [[deprecated("Use TryUniqueID()")]]
        int32_t UniqueID() const { return TryUniqueID().value_or(-1); }

        std::optional<bool> TryIsDormant() const {
            if (!is_valid()) return std::nullopt;
            auto byte_opt = maxi::driver::g_driver->try_read<uint8_t>(m_base_address + memory_map::Dormant);
            if (!byte_opt) return std::nullopt;
            return (*byte_opt & 0x1) != 0;
        }

        [[deprecated("Use TryIsDormant()")]]
        bool isDormant() const { return TryIsDormant().value_or(false); }

        std::optional<uint8_t> TryDormantByte() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<uint8_t>(m_base_address + memory_map::Dormant);
        }

        [[deprecated("Use TryDormantByte()")]]
        uint8_t DormantByte() const { return TryDormantByte().value_or(0); }

        std::optional<uintptr_t> TryMesh() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::Mesh);
        }

        [[deprecated("Use TryMesh()")]]
        uintptr_t Mesh() const { return TryMesh().value_or(0); }

        int32_t BoneCount(uintptr_t mesh) const {
            if (!is_valid()) return 0;
            if (!mesh) mesh = Mesh();
            if (!mesh) return 0;

            auto bone_count = maxi::driver::g_driver->read<int32_t>(mesh + memory_map::BoneArray + memory_map::BoneCountOffset);
            if(!bone_count)
                bone_count = maxi::driver::g_driver->read<int32_t>(mesh + memory_map::BoneArrayCache + memory_map::BoneCountOffset);

            if (bone_count <= 0 || bone_count > 512)
                return 0;

            return bone_count;
        }




        static bool MeshHasBodyBone(uintptr_t mesh) {
            auto drv = maxi::driver::g_driver;
            if (!drv || !mesh) return false;
            static std::unordered_map<uintptr_t, bool> s_cache;
            auto it = s_cache.find(mesh);
            if (it != s_cache.end()) return it->second;
            auto up = [](uint64_t p) { return p >= 0x10000ULL && (p >> 47) == 0; };
            uint64_t asset = 0;
            { auto a = drv->try_read<uint64_t>(mesh + 0xA58); if (a && up(*a)) asset = *a;
              else { auto b = drv->try_read<uint64_t>(mesh + 0x708); if (b && up(*b)) asset = *b; } }
            bool body = false;
            if (asset) {
                std::vector<uint8_t> tab(0xF80, 0);
                for (size_t o = 0; o < tab.size(); o += 0x100) {
                    const size_t c = (tab.size() - o < 0x100) ? (tab.size() - o) : 0x100;
                    drv->read_memory(asset + 0x80 + o, tab.data() + o, c);
                }
                auto rd64 = [&](uint32_t off) { uint64_t v = 0; std::memcpy(&v, tab.data() + (off - 0x80), 8); return v; };
                auto rd32 = [&](uint32_t off) { int32_t v = 0; std::memcpy(&v, tab.data() + (off - 0x80), 4); return v; };
                static const struct { uint32_t noff, stride; } L[] = {{0,12},{0,16},{0,20},{0,24}};
                int arrays_scanned = 0;
                for (uint32_t off = 0x80; off + 16 <= 0x1000 && !body && arrays_scanned < 12; off += 8) {
                    uint64_t d = rd64(off); if (!up(d)) continue;
                    int32_t n = rd32(off + 8); if (n < 16 || n > 512) continue;
                    ++arrays_scanned;
                    for (const auto& l : L) {
                        int hits = 0;
                        for (int i = 0; i < 18 && i < n; ++i) {
                            auto ci = drv->try_read<int32_t>(d + (uintptr_t)i * l.stride + l.noff);
                            if (!ci || !*ci) continue;
                            std::string nm = UObject::resolve_fname_key(*ci);
                            for (auto& c : nm) if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
                            if (nm == "pelvis" || nm == "head" || nm == "neck" || nm.find("spine") != std::string::npos) ++hits;
                        }
                        if (hits >= 2) { body = true; break; }
                    }
                }
            }
            if (s_cache.size() > 512) s_cache.clear();
            s_cache[mesh] = body;
            return body;
        }


        std::optional<uintptr_t> TryResolveMesh(uintptr_t& out_bone_array) const {
            if (!is_valid()) return std::nullopt;
            const uintptr_t mesh_cands[] = {
                memory_map::Mesh,
                memory_map::MeshCharacter,
                memory_map::Mesh1P,
                memory_map::MeshOverlay1P
            };
            uintptr_t fallback_m = 0, fallback_ba = 0;
            for (uintptr_t mo : mesh_cands) {
                if (!mo) continue;
                auto m_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + mo);
                if (!m_opt || !*m_opt) continue;
                const uintptr_t m = *m_opt;
                uintptr_t ba = maxi::driver::g_driver->try_read<uintptr_t>(m + memory_map::BoneArray).value_or(0);
                if (!ba) ba = maxi::driver::g_driver->try_read<uintptr_t>(m + memory_map::BoneArrayCache).value_or(0);
                if (!ba) continue;
                int32_t bc = maxi::driver::g_driver->read<int32_t>(m + memory_map::BoneArray + memory_map::BoneCountOffset);
                if (bc < 16 || bc > 512)
                    bc = maxi::driver::g_driver->read<int32_t>(m + memory_map::BoneArrayCache + memory_map::BoneCountOffset);
                if (bc < 16 || bc > 512) continue;
                if (!fallback_m) { fallback_m = m; fallback_ba = ba; }
                if (MeshHasBodyBone(m)) { out_bone_array = ba; return m; }
            }
            if (fallback_m) { out_bone_array = fallback_ba; return fallback_m; }
            return std::nullopt;
        }

        std::optional<uintptr_t> TryBoneArray() const {
            if (!is_valid()) return std::nullopt;
            if (memory_map::BoneArray > 0x10000) {
                auto base = maxi::driver::g_driver->get_base_address();
                return maxi::driver::g_driver->try_read<uintptr_t>(base + memory_map::BoneArray);
            }
            auto mesh_opt = TryMesh();
            if (!mesh_opt || !*mesh_opt) return std::nullopt;
            auto ba_opt = maxi::driver::g_driver->try_read<uintptr_t>(*mesh_opt + memory_map::BoneArray);
            if (ba_opt && *ba_opt) return ba_opt;
            return maxi::driver::g_driver->try_read<uintptr_t>(*mesh_opt + memory_map::BoneArrayCache);
        }

        [[deprecated("Use TryBoneArray()")]]
        uintptr_t BoneArray() const { return TryBoneArray().value_or(0); }

        FVector GetBoneWithRotation(uintptr_t mesh, uintptr_t bone_array, int32_t idx) const {
            if (!is_valid()) return {};
            FTransform bone = maxi::driver::g_driver->read<FTransform>(bone_array + (idx * memory_map::BoneStride));

            FTransform ComponentToWorld = maxi::driver::g_driver->read<FTransform>(mesh + memory_map::ComponentToWorld);

            FMatrix Matrix = bone.ToMatrixWithScale() * ComponentToWorld.ToMatrixWithScale();
            return FVector{ Matrix.WPlane.X, Matrix.WPlane.Y, Matrix.WPlane.Z };
        }

        std::optional<FVector> TryGetBoneWithRotation(uintptr_t mesh, uintptr_t bone_array, int32_t idx) const {
            if (!is_valid()) return std::nullopt;
            auto bone_opt = maxi::driver::g_driver->try_read<FTransform>(bone_array + (idx * memory_map::BoneStride));
            if (!bone_opt) return std::nullopt;
            auto ctw_opt = maxi::driver::g_driver->try_read<FTransform>(mesh + memory_map::ComponentToWorld);
            if (!ctw_opt) return std::nullopt;
            FMatrix Matrix = bone_opt->ToMatrixWithScale() * ctw_opt->ToMatrixWithScale();
            return FVector{ Matrix.WPlane.X, Matrix.WPlane.Y, Matrix.WPlane.Z };
        }

        [[deprecated("Use TryGetBoneWithRotation() instead.")]]
        FVector GetBoneWithRotation_compat(uintptr_t mesh, uintptr_t bone_array, int32_t idx) const {
            return TryGetBoneWithRotation(mesh, bone_array, idx).value_or(FVector{});
        }



        std::optional<std::vector<FVector>> TryGetBonesWithSharedCTW(uintptr_t mesh, uintptr_t bone_array,
                                                   const std::vector<int32_t>& bone_indices) const {
            std::vector<FVector> result;
            if (!is_valid() || bone_indices.empty() || !bone_array) return std::nullopt;

            FTransform ComponentToWorld = maxi::driver::g_driver->read<FTransform>(mesh + memory_map::ComponentToWorld);

            result.reserve(bone_indices.size());

            std::vector<batch_read_entry> entries(bone_indices.size());
            std::vector<FTransform> bones(bone_indices.size());
            for (size_t i = 0; i < bone_indices.size(); ++i) {
                entries[i].address = bone_array + (static_cast<uintptr_t>(bone_indices[i]) * memory_map::BoneStride);
                entries[i].size = sizeof(FTransform);
                entries[i].offset = static_cast<uint32_t>(i * sizeof(FTransform));
            }
            if (g_vm->read_batch(entries.data(), static_cast<uint32_t>(entries.size()),
                                 bones.data(), bones.size() * sizeof(FTransform))) {
                for (size_t i = 0; i < bones.size(); ++i) {
                    FMatrix Matrix = bones[i].ToMatrixWithScale() * ComponentToWorld.ToMatrixWithScale();
                    result.emplace_back(Matrix.WPlane.X, Matrix.WPlane.Y, Matrix.WPlane.Z);
                }
                return result;
            }

            std::vector<std::optional<FVector>> fallback_result;
            fallback_result.reserve(bone_indices.size());
            bool all_succeeded = true;
            for (int32_t idx : bone_indices) {
                auto bone_opt = maxi::driver::g_driver->try_read<FTransform>(bone_array + (idx * memory_map::BoneStride));
                if (!bone_opt) {
                    all_succeeded = false;
                    fallback_result.emplace_back(std::nullopt);
                    continue;
                }
                FMatrix Matrix = bone_opt->ToMatrixWithScale() * ComponentToWorld.ToMatrixWithScale();
                fallback_result.emplace_back(FVector{ Matrix.WPlane.X, Matrix.WPlane.Y, Matrix.WPlane.Z });
            }
            if (!all_succeeded) {
                return std::nullopt;
            }
            std::vector<FVector> fallback_vec;
            fallback_vec.reserve(fallback_result.size());
            for (auto& opt : fallback_result) {
                if (opt) fallback_vec.push_back(*opt);
            }
            return fallback_vec;
        }

        [[deprecated("Use TryGetBonesWithSharedCTW() instead.")]]
        std::vector<FVector> GetBonesWithSharedCTW(uintptr_t mesh, uintptr_t bone_array,
                                                    const std::vector<int32_t>& bone_indices) const {
            return TryGetBonesWithSharedCTW(mesh, bone_array, bone_indices).value_or(std::vector<FVector>{});
        }


std::optional<std::vector<FVector>> TryGetMultipleBonesWithRotation(uintptr_t mesh, uintptr_t bone_array,
                                                          const std::vector<int32_t>& bone_indices) const {
            if (!is_valid() || bone_indices.empty()) return std::nullopt;

            auto ctw_opt = maxi::driver::g_driver->try_read<FTransform>(mesh + memory_map::ComponentToWorld);
            if (!ctw_opt) return std::nullopt;
            FTransform ComponentToWorld = *ctw_opt;


            int32_t min_idx = *std::min_element(bone_indices.begin(), bone_indices.end());
            int32_t max_idx = *std::max_element(bone_indices.begin(), bone_indices.end());
            int32_t bone_count = max_idx - min_idx + 1;


            auto bones_batch_opt = maxi::driver::g_driver->try_read_array_batch<FTransform>(
                bone_array + (min_idx * memory_map::BoneStride), bone_count);

            if (!bones_batch_opt || bones_batch_opt->size() != bone_count) return std::nullopt;
            const auto& bones_batch = *bones_batch_opt;

            std::vector<FVector> result;
            result.reserve(bone_indices.size());

            for (int32_t idx : bone_indices) {
                int32_t local_idx = idx - min_idx;
                if (local_idx >= 0 && local_idx < bone_count) {
                    FMatrix Matrix = bones_batch[local_idx].ToMatrixWithScale() * ComponentToWorld.ToMatrixWithScale();
                    result.emplace_back(Matrix.WPlane.X, Matrix.WPlane.Y, Matrix.WPlane.Z);
                } else {
                    return std::nullopt;
                }
            }

            return result;
        }

        [[deprecated("Use TryGetMultipleBonesWithRotation() instead.")]]
        std::vector<FVector> GetMultipleBonesWithRotation(uintptr_t mesh, uintptr_t bone_array,
                                                          const std::vector<int32_t>& bone_indices) const {
            return TryGetMultipleBonesWithRotation(mesh, bone_array, bone_indices).value_or(std::vector<FVector>{});
        }



        std::optional<std::vector<FVector>> TryGetBonesBatchStride(uintptr_t mesh, uintptr_t bone_array,
                                                 const std::vector<int32_t>& bone_indices,
                                                 uintptr_t stride) const {
            if (!is_valid() || bone_indices.empty() || !bone_array || !stride) return std::nullopt;

            auto ctw_opt = maxi::driver::g_driver->try_read<FTransform>(mesh + memory_map::ComponentToWorld);
            if (!ctw_opt) return std::nullopt;
            FTransform ComponentToWorld = *ctw_opt;

            int32_t min_idx = *std::min_element(bone_indices.begin(), bone_indices.end());
            int32_t max_idx = *std::max_element(bone_indices.begin(), bone_indices.end());
            int32_t span = max_idx - min_idx + 1;

            const size_t total_size = static_cast<size_t>(span) * stride;
            std::vector<std::uint8_t> buffer(total_size);
            if (!maxi::driver::g_driver->read_memory(
                    bone_array + (static_cast<uintptr_t>(min_idx) * stride),
                    buffer.data(), total_size)) {
                return std::nullopt;
            }

            std::vector<FVector> result;
            result.reserve(bone_indices.size());
            for (int32_t idx : bone_indices) {
                int32_t local = idx - min_idx;
                if (local < 0 || local >= span) return std::nullopt;
                FTransform bone;
                std::memcpy(&bone, buffer.data() + (static_cast<size_t>(local) * stride), sizeof(FTransform));
                FMatrix Matrix = bone.ToMatrixWithScale() * ComponentToWorld.ToMatrixWithScale();
                result.emplace_back(Matrix.WPlane.X, Matrix.WPlane.Y, Matrix.WPlane.Z);
            }
            return result;
        }

        [[deprecated("Use TryGetBonesBatchStride() instead.")]]
        std::vector<FVector> GetBonesBatchStride(uintptr_t mesh, uintptr_t bone_array,
                                                 const std::vector<int32_t>& bone_indices,
                                                 uintptr_t stride) const {
            return TryGetBonesBatchStride(mesh, bone_array, bone_indices, stride).value_or(std::vector<FVector>{});
        }

        std::optional<uintptr_t> TryComponentToWorld() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::ComponentToWorld);
        }

        [[deprecated("Use TryComponentToWorld()")]]
        uintptr_t ComponentToWorld() const { return TryComponentToWorld().value_or(0); }

        std::optional<ADamageHandler> TryDamageHandler() const {
            if (!is_valid()) return std::nullopt;
            auto dh_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::DamageHandler);
            if (!dh_opt || !*dh_opt) return std::nullopt;
            return ADamageHandler(*dh_opt);
        }

        [[deprecated("Use TryDamageHandler()")]]
        ADamageHandler DamageHandler() const { return TryDamageHandler().value_or(ADamageHandler{}); }

        std::optional<APlayerState> TryPlayerState() const {
            if (!is_valid()) return std::nullopt;
            auto ps_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::PlayerState);
            if (!ps_opt || !*ps_opt) return std::nullopt;
            return APlayerState(*ps_opt);
        }

        [[deprecated("Use TryPlayerState()")]]
        APlayerState PlayerState() const { return TryPlayerState().value_or(APlayerState{}); }

        std::optional<bool> TryWasAlly() const {
            if (!is_valid()) return std::nullopt;
            return maxi::driver::g_driver->try_read<bool>(m_base_address + memory_map::bWasAlly);
        }

        [[deprecated("Use TryWasAlly() - distinguishes false (enemy) from read failure")]]
        bool wasAlly() const { return TryWasAlly().value_or(false); }

        std::optional<int32_t> TryGetTeamId() const {
            if (!is_valid()) return std::nullopt;
            auto tc_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::TeamComponent);
            if (!tc_opt || !*tc_opt) return std::nullopt;
            return maxi::driver::g_driver->try_read<int32_t>(*tc_opt + memory_map::TeamId);
        }

        [[deprecated("Use TryGetTeamId() - distinguishes -1 (unknown) from read failure")]]
        int32_t GetTeamId() const { return TryGetTeamId().value_or(-1); }

        std::optional<AShooterCharacterMinimapComponent> TryMinimapComponent() const {
            if (!is_valid()) return std::nullopt;
            auto mc_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::CharacterMinimap);
            if (!mc_opt || !*mc_opt) return std::nullopt;
            return AShooterCharacterMinimapComponent(*mc_opt);
        }

        [[deprecated("Use TryMinimapComponent()")]]
        AShooterCharacterMinimapComponent MinimapComponent() const { return TryMinimapComponent().value_or(AShooterCharacterMinimapComponent{}); }

        std::optional<bool> TryIsVisible(uintptr_t Mesh, uintptr_t local_player_addr = 0) const {
            if (!is_valid()) return std::nullopt;



            struct VisEnt { uintptr_t mm = 0; uintptr_t vc = 0; };
            static thread_local std::unordered_map<uintptr_t, VisEnt> s_visent;
            if (s_visent.size() > 8192) s_visent.clear();
            VisEnt& e = s_visent[m_base_address];

            uintptr_t mm_addr = e.mm;
            if (!mm_addr) {
                auto minimap_opt = TryMinimapComponent();
                if (minimap_opt && minimap_opt->is_valid()) {
                    mm_addr = minimap_opt->get_base_address();
                    e.mm = mm_addr;
                }
            }
            if (mm_addr) {
                auto bv = maxi::driver::g_driver->try_read<bool>(mm_addr + memory_map::MinimapVisible);
                if (bv) return *bv;
                e.mm = 0; e.vc = 0;
            }


            auto lrt_opt = maxi::driver::g_driver->try_read<float>(Mesh + memory_map::LastRenderTime);
            auto lst_opt = maxi::driver::g_driver->try_read<float>(Mesh + memory_map::LastSubmitTime);
            if (!lrt_opt || !lst_opt) return std::nullopt;
            float visionTick = *lrt_opt + 0.02f;
            return visionTick >= *lst_opt;
        }

        [[deprecated("Use TryIsVisible() - distinguishes false (invisible) from read failure")]]
        bool IsVisible(uintptr_t Mesh) const { return TryIsVisible(Mesh).value_or(false); }

        std::optional<UObject> TryCurrentEquippable() const {
            if (!is_valid()) return std::nullopt;
            auto inv_opt = maxi::driver::g_driver->try_read<uintptr_t>(m_base_address + memory_map::Inventory);
            if (!inv_opt || !*inv_opt) return std::nullopt;
            return maxi::driver::g_driver->try_read<UObject>(*inv_opt + memory_map::CurrentEquippable);
        }

        [[deprecated("Use TryCurrentEquippable()")]]
        UObject CurrentEquippable() const { return TryCurrentEquippable().value_or(UObject{}); }
    };
}