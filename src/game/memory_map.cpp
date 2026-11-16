#include "game/memory_map.hpp"

namespace memory_map {
    namespace vgx {
        uintptr_t ShadowRegions = 0x819C8;
        uintptr_t ShadowRegionQ = 0x819E0;
        uintptr_t ShadowRegionB = 0x81A90;
    }

    namespace old_vgx {
        uintptr_t ShadowRegions = 0x838F8;
        uintptr_t ShadowRegionQ = 0x83910;
        uintptr_t ShadowRegionB = 0x839C0;
    }


    uintptr_t VgkPool1 = 0x0027;



    uintptr_t FNamePool = 0xD9C9B40;
    uintptr_t FNameState = 0xD8CF4D0;
    uint64_t FNameKeyStatic = 0x00000000151E547Cull;

    uintptr_t GWorld = 0xD82CC70;
    uintptr_t GObjects = 0xCF4FC50;
    uintptr_t uworld_state = 0xD82CC70;
    uintptr_t OwningWord = 0x00C0;
    uintptr_t PersistentLevel = 0x0038;
    uintptr_t OwningGameInstance = 0x01D8;
    uintptr_t GameState = 0x0178;
    uintptr_t LocalPlayers = 0x0040;
    uintptr_t PlayerController = 0x0038;
    uintptr_t PlayerCameraManager = 0x0528;
    uintptr_t AcknowledgedPawn = 0x0518;
    uintptr_t PawnPrivate = 0x04E8;
    uintptr_t PlayerState = 0x0488;
    uintptr_t CameraCache = 0x17B0;
    uintptr_t ActorArray = 0x00A0;
    uintptr_t ControlRotation = 0x04E8;

    uintptr_t UniqueID = 0x0038;
    uintptr_t FNameId = 0x0018;
    uintptr_t ActorId = 0x011C;
    uintptr_t ActorDestroyed = 0x0120;
    uintptr_t IsBeingDestroyed = 0x011C;
    uintptr_t Dormant = 0x0225;
    uintptr_t Mesh = 0x04F0;

    uintptr_t MeshCharacter = 0x04F0;
    uintptr_t Mesh1P = 0x0F10;
    uintptr_t MeshOverlay1P = 0x0F18;
    uintptr_t Inventory = 0x0C08;
    uintptr_t CurrentEquippable = 0x0278;
    uintptr_t EquippableVFXState = 0x0E08;
    uintptr_t InstanceComponents = 0x0438;
    uintptr_t BlueprintCreatedComponents = 0x0448;
    uintptr_t LastRenderTime = 0x04AC;
    uintptr_t LastSubmitTime = 0x04A8;
    uintptr_t CharacterMinimap = 0x14F8;
    uintptr_t ComponentToWorld = 0x0300;
    uintptr_t RootComponent = 0x0290;
    uintptr_t RelativeScale3D = 0x01A0;
    uintptr_t AttachChildren = 0x0100;
    uintptr_t AttachChildrenCount = 0x0108;
    uintptr_t OutlineMode = 0x0351;
    uintptr_t TeamComponent = 0x06A8;
    uintptr_t DamageHandler = 0x0C70;
    uintptr_t TeamId = 0x00F8;
    uintptr_t bWasAlly = 0x0F58;
    uintptr_t RelativeLocation = 0x0170;
    uintptr_t RootPosition = 0x0170;
    uintptr_t RelativeRotation = 0x0188;
    uintptr_t Health = 0x0200;
    uintptr_t Shield = 0x0208;
    uintptr_t MaxShield = 0x020C;
    uintptr_t ShieldType = 0x0210;
    uintptr_t BoneArray = 0x0760;
    uintptr_t BoneArrayCache = BoneArray + 0x10;
    uintptr_t BoneCount = 0x0768;
    uintptr_t BombTimer = 0x05A8;
    uintptr_t BombDefuseProgress = 0x05D0;
    uintptr_t DefuseStage = 0x05F0;
    uintptr_t SpikePlantedMaw = 0x05B8;



    uintptr_t BoneStride = 0x0060;
    uintptr_t BoneCountOffset = 0x0008;


    uintptr_t UObjectVtable = 0x0000;
    uintptr_t UObjectFlags = 0x0008;
    uintptr_t UObjectIndex = 0x000C;
    uintptr_t UObjectClass = 0x0010;
    uintptr_t UObjectName = 0x0018;
    uintptr_t UObjectOuter = 0x0020;
    uintptr_t UObjectSuper = 0x0048;


    uintptr_t     VisibilityLostTime = 0x016C;

    uintptr_t MinimapVisible = 0x0689;
    uintptr_t MinimapVisibilityComponent = 0x14F8;
    uintptr_t MinimapBIsAlive = 0x0687;
    uintptr_t MinimapBHasBomb = 0x0688;
    uintptr_t MinimapBIsVisible = 0x0689;



    uintptr_t VisibilityComponentOffset = 0x0518;
    uintptr_t CharactersWithVisibilityOffset = 0x0118;


    uintptr_t ViewTargetPOV = 0x0010;
    uintptr_t POVLocation = 0x0000;
    uintptr_t POVRotation = 0x0018;
    uintptr_t POVFOV = 0x0030;
    uintptr_t CameraPosition = 0x17C0;
    uintptr_t ViewInfo = 0x17C0;
    uintptr_t CameraTimestamp = 0x0000;
    uintptr_t Angle = 0x0170;
    uintptr_t WorldGuarded = 0x0130;


    uintptr_t StateKey = 0x0038;
    uintptr_t StateKeyAltShift = 0x0008;


    uintptr_t GWorldAlt1 = 0xD173910;
    uintptr_t GWorldAlt2 = 0xD173910;
    uintptr_t GWorldAlt3 = 0xCEC5640;
    uintptr_t GWorldAlt4 = 0xCFD1D00;
    uintptr_t GWorldAlt5 = 0xCEC9500;



    const std::array<uintptr_t, 5> GWorldPml4Candidates = {
        GWorld, uworld_state, GWorldAlt1, GWorldAlt2, GWorldAlt3
    };

    const std::array<uintptr_t, 7> UWorldStateCandidates = {
        uworld_state, GWorld, GWorldAlt1, GWorldAlt2, GWorldAlt3, GWorldAlt4, GWorldAlt5
    };

    const std::array<uintptr_t, 4> GWorldStaticCandidates = {
        GWorld, GWorldAlt1, GWorldAlt2, GWorldAlt3
    };

    const std::array<uintptr_t, 11> PersistentLevelFallbacks = { 0x0280, 0x0040, 0x0048, 0x0050, 0x0030, 0x0028, 0x0098, 0x0128, 0x01F0, 0x0398, 0x0008 };

    const std::array<uintptr_t, 3> LocalPlayersCandidates = { LocalPlayers, 0x0038, 0x0048 };

    const std::array<uintptr_t, 12> ActorArrayCandidates = {
        ActorArray, 0x05F0, 0x0410, 0x1478, 0x19F8, 0x00A8, 0x00B0, 0x00B8, 0x0098, 0x0090, 0x0088, 0x0390
    };


    uintptr_t TriggerVEH = 0x18EF3D6;
    uintptr_t fmemory_malloc = 0x18B7C50;
    uintptr_t mount = 0x1D181D0;
    uintptr_t static_find_object = 0x1D11C80;
    uintptr_t static_load_object = 0x1D157B0;
    uintptr_t process_event = 0x1CE59E0;
    uintptr_t set_ares_outline_mode = 0x4207EC0;
    uintptr_t play_finisher_effect = 0x6FFC940;
    uintptr_t get_spread_values = 0x70A3E30;
    uintptr_t get_spread_angles = 0x7E92170;
    uintptr_t to_vector_and_normalize = 0x19B7AE0;
    uintptr_t to_angle_and_normalize = 0x19B2280;
    uintptr_t get_firing_location_and_direction = 0x7940AC0;
    uintptr_t bone_matrix = 0x4274A80;
    uintptr_t fname_pool_looper = 0x1B9FB80;
    uintptr_t SigningDelegate = 0xDBDD000;
    uintptr_t FPakPlatformFile = 0xD871E78;

    uintptr_t SpectatedPlayer = 0x0748;
    uintptr_t PlayerNamePrivate = 0x0508;
    uintptr_t PlatformPlayer = 0x06A0;
    uintptr_t GetPing = 0x046C;
    uintptr_t AccountLevel = 0x0794;
    uintptr_t CompetitiveTier = 0x07A4;
    uintptr_t OutlineAllyColor = 0x52D96C2;
    uintptr_t OutlineEnemyColor = 0x52D96C9;

    namespace derive {
        uint64_t UObjectMixKey = 0x2545F4914F6CDD1DULL;
        static_assert(UObjectStateSlots == 7, "UObjectStateSlots must stay 7 (keys[] layout)");

        uint64_t ByteMask = 0x73ULL;
        uint64_t VgkK1 = 0x8000000000000001ULL;
        uint64_t VgkK2 = 0xAFDBF65F8A4AC9C9ULL;
        uint64_t VgkK3 = 0x2FDBF65F8A4AC9C9ULL;
        uint64_t VgkK4 = 0x8000000000000000ULL;
        uint64_t VgkK5 = 0x7D90DC33C620C593ULL;
        uint64_t VgkK6 = 0x13D0F34E00000000ULL;
        uint64_t VgkK7 = 0x483C4F8900000000ULL;
        uint64_t VgkK8 = 0xFD90DC33C620C592ULL;
        uint64_t VgkK9 = 0xCE3CE5E180000000ULL;
        uint64_t VgkK10 = 0x55494E5B80000000ULL;
        uint64_t VgkK11 = 0xC83B18136241A38DULL;
        uint64_t VgkK12 = 0x72F1C9B7E241A38DULL;
        uint64_t VgkK13 = 0x3F71D992FBB2CCEBULL;
        uint64_t VgkK14 = 0x71C31A1E80000000ULL;
        uint64_t VgkK15 = 0x99BF7D2380CF6EC3ULL;
        uint64_t VgkK16 = 0x664082DC7F30913EULL;
        uint64_t VgkK17 = 0x19BF7D2380CF6EC2ULL;
        uint64_t VgkK18 = 0xE64082DC7F30913EULL;
        uint64_t VgkK19 = 0x2183995CC620C592ULL;
        uint64_t VgkK20 = 0x49B74B6480000000ULL;
        uint64_t VgkK21 = 0xC2D8B464B4418C6CULL;
        uint64_t VgkK22 = 0x66B8CDC1FFFFFFFFULL;
        uint64_t VgkK23 = 0x5C1FE6A2B4418C6DULL;
        uint64_t VgkK24 = 0xC08E266D044D3314ULL;
        uint64_t VgkK25 = 0x6C80110954C7304DULL;
        uint64_t VgkK26 = 0x137FEEF6AB38CFB4ULL;
        uint64_t VgkK27 = 0x4F167C5CD4C7304EULL;
        uint64_t VgkK28 = 0x7FFFFFFFFFFFFFFFULL;
    }
}
