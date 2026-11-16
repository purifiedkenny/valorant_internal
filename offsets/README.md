# Offsets - VALORANT 13.10

Game: VALORANT. Offset set: **13.10**. Definitions live in
`src/game/memory_map.cpp` and `include/game/memory_map.hpp`. All reads are
performed through the kernel driver (user CR3).

Offsets change with game updates. Update `memory_map.cpp` and this table
together; do not mix values from different builds.

---

## 1. Core chain

| Offset | Value (13.10) | Description |
|---|---|---|
| `GWorld` | `base + 0xD82CC70` | Plain `UWorld*`, NO decrypt |
| `uworld_state` | `base + 0xD82CC70` | GWorld fallback / UWorld state candidate (same address) |
| `GObjects` | `base + 0xCF4FC50` | UObject pool |
| `FNamePool` | `base + 0xD9C9B40` | FName pool |
| `FNameState` | `base + 0xD8CF4D0` | FNameState |
| `FNameState + StateKey` | `base + 0xD8CF508` | Derived: `FNameState` (`0xD8CF4D0`) + `StateKey` (`0x38`) |
| `FNameKeyStatic` | `0x151E547C` | Static fallback key |
| `FPakPlatformFile` | `base + 0xD871E78` | Pak platform file |
| `SigningDelegate` | `base + 0xDBDD000` | Signing delegate |
| `vgx::ShadowRegions` | `0x819C8` | Vanguard shadow region base (kernel base offset) |
| `VgkPool1` | `0x27` | VGK pool secondary offset |

> The GWorld pointer can fluctuate between two UWorld instances. The code
> filters out stale worlds with `level_present()` validation.

## 2. UWorld -> Player chain

| Offset | Value | Description |
|---|---|---|
| `OwningGameInstance` | `+0x01D8` | UWorld -> GameInstance |
| `PersistentLevel` | `+0x0038` | UWorld -> ULevel |
| `GameState` | `+0x0178` | UWorld/GameInstance -> GameState |
| `LocalPlayers` | `+0x0040` | GameInstance -> LocalPlayers[0] |
| `PlayerController` | `+0x0038` | LocalPlayer -> APlayerController |
| `PlayerCameraManager` | `+0x0528` | PC -> PCM |
| `AcknowledgedPawn` | `+0x0518` | PC -> local pawn |
| `PawnPrivate` | `+0x04E8` | PlayerState -> Pawn |
| `CameraCache` | `+0x17B0` | PCM -> CameraCache |
| `ViewInfo` | `+0x17C0` | Camera view info |
| `ActorArray` | `+0x00A0` | Level -> TArray<AActor> |
| `ActorArrayCount` | `+0x00A8` | Derived: `ActorArray` (`+0x00A0`) + 0x8 (TArray count; no standalone constant) |
| `ControlRotation` | `+0x04E8` | PC -> control rotation |

## 3. Actor (AActor)

| Offset | Value | Description |
|---|---|---|
| `Mesh` | `+0x04F0` | ACharacter.Mesh (1P=0xF10, Overlay=0xF18) |
| `Inventory` | `+0x0C08` | weapon inventory |
| `CurrentEquippable` | `+0x0278` | Inventory -> current weapon |
| `EquippableVFXState` | `+0x0E08` | equippable VFX state |
| `DamageHandler` | `+0x0C70` | damage handler (health/shield) |
| `bWasAlly` | `+0x0F58` | whether actor was a teammate |
| `UniqueID` | `+0x0038` | unique ID |
| `FNameId` | `+0x0018` | FName index |
| `ActorId` | `+0x011C` | actor ID |
| `ActorDestroyed` | `+0x0120` | destroyed flag |
| `IsBeingDestroyed` | `+0x011C` | being-destroyed flag |
| `Dormant` | `+0x0225` | dormant flag |
| `TeamComponent` | `+0x06A8` | team component |
| `TeamId` | `+0x00F8` | team ID |
| `RootComponent` | `+0x0290` | root component |
| `RootPosition` | `+0x0170` | root position |
| `RelativeScale3D` | `+0x01A0` | scale |
| `AttachChildren` | `+0x0100` | attached children |
| `AttachChildrenCount` | `+0x0108` | attached child count |
| `OutlineMode` | `+0x0351` | outline mode |

## 4. Mesh (SkeletalMeshComponent)

| Offset | Value | Description |
|---|---|---|
| `BoneArray` | `+0x0760` | bone transform array pointer |
| `BoneArrayCache` | `+0x0770` | Derived: `BoneArray + 0x10` (fallback) |
| `BoneCount` | `+0x0768` | bone count |
| `BoneStride` | `0x60` | stride per bone |
| `ComponentToWorld` | `+0x0300` | 4x4 transform |
| `LastRenderTime` | `+0x04AC` | visibility (visible check) |
| `LastSubmitTime` | `+0x04A8` | visibility threshold |

## 5. DamageHandler

| Offset | Value | Description |
|---|---|---|
| `Health` | `+0x0200` | health |
| `Shield` | `+0x0208` | shield |
| `MaxShield` | `+0x020C` | max shield |
| `ShieldType` | `+0x0210` | shield type |

## 6. Camera (POV)

| Offset | Value | Description |
|---|---|---|
| `ViewTargetPOV` | `+0x0010` | CameraCache -> POV |
| `POVLocation` | `+0x0000` | FVector (24B) |
| `POVRotation` | `+0x0018` | FRotator (12B) |
| `POVFOV` | `+0x0030` | float (horizontal FOV) |
| `CameraPosition` | `+0x17C0` | camera position |
| `CameraTimestamp` | `+0x0000` | timestamp |
| `Angle` | `+0x0170` | view angle |
| `WorldGuarded` | `+0x0130` | guarded world flag |

The `FMinimalViewInfo` struct matches this offset layout exactly, so a single
`read<FMinimalViewInfo>` is performed.

## 7. Spike / Objectives

| Offset | Value | Description |
|---|---|---|
| `SpikePlantedMaw` | `+0x05B8` | spike planted |
| `BombTimer` | `+0x05A8` | spike detonation time |
| `BombDefuseProgress` | `+0x05D0` | defuse progress |

## 8. Player profile

| Offset | Value | Description |
|---|---|---|
| `GetPing` | `+0x046C` | ping |
| `PlatformPlayer` | `+0x06A0` | platform player |
| `AccountLevel` | `+0x0794` | account level |
| `CompetitiveTier` | `+0x07A4` | competitive tier |
| `OutlineAllyColor` | `0x52D96C2` | ally outline color |
| `OutlineEnemyColor` | `0x52D96C9` | enemy outline color |

## 9. Engine functions

| Function | Value |
|---|---|
| `process_event` | `0x1CE59E0` |
| `static_find_object` | `0x1D11C80` |
| `static_load_object` | `0x1D157B0` |
| `bone_matrix` | `0x4274A80` |
| `set_ares_outline_mode` | `0x4207EC0` |
| `fmemory_malloc` | `0x18B7C50` |
| `play_finisher_effect` | `0x6FFC940` |
| `fname_pool_looper` | `0x1B9FB80` |
| `get_spread_values` | `0x70A3E30` |
| `get_spread_angles` | `0x7E92170` |
| `to_vector_and_normalize` | `0x19B7AE0` |
| `to_angle_and_normalize` | `0x19B2280` |
| `get_firing_location_and_direction` | `0x7940AC0` |
| `TriggerVEH` | `0x18EF3D6` |
| `mount` | `0x1D181D0` |

## 10. Bone map

struct order: `{head, neck, chest, lUpperArm, lForeArm, lHand, rUpperArm,
rForeArm, rHand, stomach, pelvis, lThigh, lKnee, lFoot, rThigh, rKnee, rFoot}`

| bone_count | Map |
|---|---|
| **106** (Brimstone) | `8,7,6, 25,26,27, 51,52,53, 3,76, 79,80,82, 86,87,89` |
| **103** (Sage) | `8,7,6, 25,26,27, 50,51,52, 3,74, 77,78,80, 84,85,87` |
| **104** | `8,21,6, 23,24,25, 49,50,51, 4,3, 77,78,80, 84,85,87` |
| **101** | `8,21,6, 23,24,25, 49,50,51, 4,3, 75,76,78, 82,83,85` |

Unknown bone_count -> per-bone_array automatic discovery (Z-band filtered).
