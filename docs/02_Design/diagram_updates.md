# Architecture Diagrams - Corrected Version

> [!IMPORTANT]
> **Architecture Updated**: These diagrams reflect the corrected architecture after critical fixes:
> - `PlaybackStatus` enum (not `PlaybackState`) to avoid recursive type definition
> - `PlaybackState` has `playQueue_` and `queueIndex_` for Next functionality  
> - `PlaybackController` has `IHardwareInterface* hardware_` for LCD write access
> 
> See [critical_fixes.md](file:///wsl.localhost/Ubuntu/home/duong/music_player/docs/02_Design/critical_fixes.md) for detailed explanations.

---

All diagrams in [architecture_diagrams.md](file:///wsl.localhost/Ubuntu/home/duong/music_player/docs/02_Design/architecture_diagrams.md) have been updated to reflect these fixes.

## Key Updates Made:

### Class Diagram (Diagram #2)
✅ Added `PlaybackStatus` enumeration  
✅ Updated `PlaybackState` to show:
  - `status: PlaybackStatus` (not `state: PlaybackState`)
  - `playQueue: vector<MediaFile>`
  - `queueIndex: size_t`
  - New methods: `setPlayQueue()`, `getNextTrack()`, `hasNextTrack()`

✅ Updated `PlaybackController` to show:
  - `hardware: IHardwareInterface*` (new dependency)
  - `sendMetadataToHardware()` method

✅ Updated `SDL2PlaybackEngine` to show:
  - `state: PlaybackStatus`
  - `mutex: mutex`

✅ Added relationships:
  - `PlaybackController` → `IHardwareInterface` (solid line for writes)
  - `PlaybackController` ..> `S32K144Interface` (dotted line for observes)
  - `PlaybackState` → `PlaybackStatus` (uses)

### All Other Diagrams
All sequence diagrams, state machines, and component diagrams remain valid and consistent with the corrected architecture.
