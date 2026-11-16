#pragma once

#include <array>
#include <cstdint>

namespace memory_map {
    namespace vgx {
        extern uintptr_t ShadowRegions;
        extern uintptr_t ShadowRegionQ;
        extern uintptr_t ShadowRegionB;
    }

    namespace old_vgx {
        extern uintptr_t ShadowRegions;
        extern uintptr_t ShadowRegionQ;
        extern uintptr_t ShadowRegionB;
    }

    extern uintptr_t VgkPool1;

    extern uintptr_t FNamePool;
    extern uintptr_t FNameState;



    extern uint64_t FNameKeyStatic;

    extern uintptr_t GWorld;
    extern uintptr_t GObjects;
    extern uintptr_t uworld_state;
    extern uintptr_t OwningWord;
    extern uintptr_t PersistentLevel;
    extern uintptr_t OwningGameInstance;
    extern uintptr_t GameState;
    extern uintptr_t LocalPlayers;
    extern uintptr_t PlayerController;
    extern uintptr_t PlayerCameraManager;
    extern uintptr_t AcknowledgedPawn;
    extern uintptr_t PawnPrivate;
    extern uintptr_t PlayerState;
    extern uintptr_t CameraCache;
    extern uintptr_t ActorArray;
    extern uintptr_t ControlRotation;

    extern uintptr_t UniqueID;
    extern uintptr_t FNameId;
    extern uintptr_t ActorId;
    extern uintptr_t ActorDestroyed;
    extern uintptr_t IsBeingDestroyed;
    extern uintptr_t Dormant;
    extern uintptr_t Mesh;
    extern uintptr_t MeshCharacter;
    extern uintptr_t Mesh1P;
    extern uintptr_t MeshOverlay1P;
    extern uintptr_t Inventory;
    extern uintptr_t CurrentEquippable;
    extern uintptr_t EquippableVFXState;
    extern uintptr_t InstanceComponents;
    extern uintptr_t BlueprintCreatedComponents;
    extern uintptr_t LastRenderTime;
    extern uintptr_t LastSubmitTime;
    extern uintptr_t CharacterMinimap;
    extern uintptr_t ComponentToWorld;
    extern uintptr_t RootComponent;
    extern uintptr_t RelativeScale3D;
    extern uintptr_t AttachChildren;
    extern uintptr_t AttachChildrenCount;
    extern uintptr_t OutlineMode;
    extern uintptr_t TeamComponent;
    extern uintptr_t DamageHandler;
    extern uintptr_t TeamId;
    extern uintptr_t bWasAlly;
    extern uintptr_t RelativeLocation;
    extern uintptr_t RootPosition;
    extern uintptr_t RelativeRotation;
    extern uintptr_t Health;
    extern uintptr_t Shield;
    extern uintptr_t MaxShield;
    extern uintptr_t ShieldType;
    extern uintptr_t BoneArray;
    extern uintptr_t BoneArrayCache;
    extern uintptr_t BoneCount;
    extern uintptr_t BombTimer;
    extern uintptr_t BombDefuseProgress;
    extern uintptr_t DefuseStage;
    extern uintptr_t SpikePlantedMaw;


    extern uintptr_t BoneStride;
    extern uintptr_t BoneCountOffset;


    extern uintptr_t UObjectVtable;
    extern uintptr_t UObjectFlags;
    extern uintptr_t UObjectIndex;
    extern uintptr_t UObjectClass;
    extern uintptr_t UObjectName;
    extern uintptr_t UObjectOuter;
    extern uintptr_t UObjectSuper;


    extern uintptr_t VisibilityLostTime;
    extern uintptr_t MinimapVisible;
    extern uintptr_t MinimapVisibilityComponent;
    extern uintptr_t MinimapBIsAlive;
    extern uintptr_t MinimapBHasBomb;
    extern uintptr_t MinimapBIsVisible;


    extern uintptr_t VisibilityComponentOffset;

    extern uintptr_t CharactersWithVisibilityOffset;


    extern uintptr_t ViewTargetPOV;
    extern uintptr_t POVLocation;
    extern uintptr_t POVRotation;
    extern uintptr_t POVFOV;
    extern uintptr_t CameraPosition;
    extern uintptr_t ViewInfo;
    extern uintptr_t CameraTimestamp;
    extern uintptr_t Angle;
    extern uintptr_t WorldGuarded;


    extern uintptr_t StateKey;
    extern uintptr_t StateKeyAltShift;


    extern uintptr_t GWorldAlt1;
    extern uintptr_t GWorldAlt2;
    extern uintptr_t GWorldAlt3;
    extern uintptr_t GWorldAlt4;
    extern uintptr_t GWorldAlt5;


    extern const std::array<uintptr_t, 5> GWorldPml4Candidates;
    extern const std::array<uintptr_t, 7> UWorldStateCandidates;
    extern const std::array<uintptr_t, 4> GWorldStaticCandidates;
    extern const std::array<uintptr_t, 11> PersistentLevelFallbacks;
    extern const std::array<uintptr_t, 3> LocalPlayersCandidates;
    extern const std::array<uintptr_t, 12> ActorArrayCandidates;


    extern uintptr_t TriggerVEH;
    extern uintptr_t fmemory_malloc;
    extern uintptr_t mount;
    extern uintptr_t static_find_object;
    extern uintptr_t static_load_object;
    extern uintptr_t process_event;
    extern uintptr_t set_ares_outline_mode;
    extern uintptr_t play_finisher_effect;
    extern uintptr_t get_spread_values;
    extern uintptr_t get_spread_angles;
    extern uintptr_t to_vector_and_normalize;
    extern uintptr_t to_angle_and_normalize;
    extern uintptr_t get_firing_location_and_direction;
    extern uintptr_t bone_matrix;
    extern uintptr_t fname_pool_looper;
    extern uintptr_t SigningDelegate;
    extern uintptr_t FPakPlatformFile;
    extern uintptr_t SpectatedPlayer;
    extern uintptr_t PlayerNamePrivate;
    extern uintptr_t PlatformPlayer;
    extern uintptr_t GetPing;
    extern uintptr_t AccountLevel;
    extern uintptr_t CompetitiveTier;
    extern uintptr_t OutlineAllyColor;
    extern uintptr_t OutlineEnemyColor;

    namespace derive {

        extern uint64_t UObjectMixKey;
        constexpr uint32_t UObjectStateSlots = 7;


        extern uint64_t ByteMask;
        extern uint64_t VgkK1;
        extern uint64_t VgkK2;
        extern uint64_t VgkK3;
        extern uint64_t VgkK4;
        extern uint64_t VgkK5;
        extern uint64_t VgkK6;
        extern uint64_t VgkK7;
        extern uint64_t VgkK8;
        extern uint64_t VgkK9;
        extern uint64_t VgkK10;
        extern uint64_t VgkK11;
        extern uint64_t VgkK12;
        extern uint64_t VgkK13;
        extern uint64_t VgkK14;
        extern uint64_t VgkK15;
        extern uint64_t VgkK16;
        extern uint64_t VgkK17;
        extern uint64_t VgkK18;
        extern uint64_t VgkK19;
        extern uint64_t VgkK20;
        extern uint64_t VgkK21;
        extern uint64_t VgkK22;
        extern uint64_t VgkK23;
        extern uint64_t VgkK24;
        extern uint64_t VgkK25;
        extern uint64_t VgkK26;
        extern uint64_t VgkK27;
        extern uint64_t VgkK28;
    }
}
