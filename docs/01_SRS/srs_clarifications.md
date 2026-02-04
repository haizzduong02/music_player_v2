# SRS Clarifications Summary

## Changes Made to SRS v1.0

### Overview
Updated 3 ambiguous requirements và added 1 new section để clarify conflicts và define resource limits.

---

## ✅ 1. FR-LIB-05: Library File Removal Behavior

### Before (Ambiguous):
> "Khi remove file ra khỏi Library, cảnh báo remove khỏi Playlist nếu có."

**Problems**:
- Không rõ "cảnh báo" là warning hay confirmation
- Không rõ what happens sau khi warning
- Không rõ file có còn playable từ Playlist không

### After (Clear):
> "Khi remove file ra khỏi Library, hệ thống hiển thị cảnh báo liệt kê các Playlist chứa file đó. User xác nhận để remove khỏi cả Library và tất cả Playlist, hoặc Cancel để giữ nguyên. File đã remove khỏi Library không thể play được từ Playlist."

**Clarifications**:
- ✅ Warning shows list of affected playlists
- ✅ User must confirm to proceed
- ✅ Cancel keeps file in both Library and Playlists
- ✅ Removed files become unplayable from Playlists
- ✅ Removal is atomic (all or nothing)

**Implementation Impact**:
```cpp
// LibraryController::removeMedia()
bool LibraryController::removeMedia(const std::string& filepath) {
    // 1. Find affected playlists
    auto affectedPlaylists = findPlaylistsContaining(filepath);
    
    // 2. Show warning dialog
    if (!affectedPlaylists.empty()) {
        std::string message = "File exists in playlists:\n";
        for (auto& pl : affectedPlaylists) {
            message += "- " + pl->getName() + "\n";
        }
        message += "\nRemove from Library and all Playlists?";
        
        if (!confirmDialog(message)) {
            return false;  // User cancelled
        }
    }
    
    // 3. Atomic removal
    library_->removeMedia(filepath);
    for (auto& pl : affectedPlaylists) {
        pl->removeTrack(filepath);
    }
    return true;
}
```

---

## ✅ 2. FR-PLL-09: "Now Playing" Playlist Protection

### Before (Conflicting):
> "Luôn có một Playlist mặc định có tên 'Now Playing'."
> 
> (Conflicted with FR-PLL-06: "Cho phép Delete playlist")

**Problems**:
- Can user delete "Now Playing"?
- Can user rename "Now Playing"?
- What operations are allowed on default playlist?

### After (Clear):
> "Luôn có một Playlist mặc định có tên 'Now Playing'. Playlist này **không thể delete** (FR-PLL-06 không áp dụng cho 'Now Playing'). User có thể Add/Remove tracks nhưng không thể Delete hoặc Rename playlist này."

**Clarifications**:
- ✅ "Now Playing" **CANNOT** be deleted
- ✅ "Now Playing" **CANNOT** be renamed
- ✅ User CAN add/remove tracks
- ✅ FR-PLL-06 explicitly does NOT apply to this playlist
- ✅ System must enforce protection

**Implementation Impact**:
```cpp
// PlaylistManager::deletePlaylist()
bool PlaylistManager::deletePlaylist(const std::string& name) {
    if (name == NOW_PLAYING_NAME) {
        // Cannot delete default playlist
        lastError_ = "Cannot delete 'Now Playing' playlist";
        return false;
    }
    // ... proceed with deletion
}

// PlaylistManager::renamePlaylist()
bool PlaylistManager::renamePlaylist(const std::string& oldName, 
                                     const std::string& newName) {
    if (oldName == NOW_PLAYING_NAME) {
        lastError_ = "Cannot rename 'Now Playing' playlist";
        return false;
    }
    // ... proceed with rename
}
```

**UI Impact**:
- Delete button should be disabled for "Now Playing"
- Rename option should be hidden for "Now Playing"

---

## ✅ 3. FR-HWI-05: Hardware Synchronization Strategy

### Before (Ambiguous):
> "Hệ thống ưu tiên giá trị tuyệt đối từ phần cứng khi có thay đổi."

**Problems**:
- What if GUI changes volume, then hardware changes 1ms later?
- How to handle ADC noise/drift?
- What about button vs knob priority?

### After (Clear):
> "Hệ thống ưu tiên giá trị tuyệt đối từ phần cứng khi có thay đổi. **Debouncing**: ADC volume changes < 5% trong 500ms bị bỏ qua để tránh noise. **Priority rule**: GUI volume change được ưu tiên trong 2 giây, sau đó hardware takes over. Button events (Play/Pause/Next) được xử lý ngay lập tức."

**Clarifications**:
- ✅ **Debouncing**: ADC changes < 5% within 500ms are ignored (noise filtering)
- ✅ **Priority window**: GUI gets 2-second priority after manual change
- ✅ **Button priority**: Buttons (discrete events) processed immediately
- ✅ **After timeout**: Hardware takes full control again

**Implementation Impact**:
```cpp
class PlaybackController {
private:
    std::chrono::steady_clock::time_point lastGUIVolumeChange_;
    float lastHardwareVolume_;
    
    void onHardwareVolumeChange(float newVolume) {
        // 1. Debouncing
        float delta = std::abs(newVolume - lastHardwareVolume_);
        if (delta < 0.05f) {  // < 5%
            return;  // Ignore noise
        }
        
        // 2. Priority rule
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastGUIVolumeChange_
        );
        
        if (elapsed.count() < 2) {
            return;  // GUI priority window active
        }
        
        // 3. Apply hardware volume
        setVolume(newVolume);
        lastHardwareVolume_ = newVolume;
    }
    
    void onGUIVolumeChange(float newVolume) {
        setVolume(newVolume);
        lastGUIVolumeChange_ = std::chrono::steady_clock::now();
    }
    
    void onHardwareButtonPress(HardwareCommand cmd) {
        // Buttons always processed immediately (no debouncing/priority)
        handleCommand(cmd);
    }
};
```

**Timing Diagram**:
```
t=0s:   User adjusts GUI volume → Applied immediately
t=0.5s: Hardware knob moves → IGNORED (within 2s window)
t=1.5s: Hardware knob moves → IGNORED (within 2s window)
t=2.1s: Hardware knob moves → Applied (window expired)
t=2.15s: Hardware noise (3% change) → IGNORED (< 5% debounce)
t=3s:   Button press → Applied immediately (no delay)
```

---

## ✅ 4. NEW: NFR-RES - Resource Limits Section

### Why Added:
- Performance concerns for large libraries
- Memory management for embedded target
- Prevention of DOS scenarios
- Clear boundaries for testing

### Requirements Added:

| ID | Requirement | Rationale |
|---|---|---|
| **NFR-RES-01** | Maximum 10,000 files per Library | Typical music collection size. Linear search acceptable at this scale. |
| **NFR-RES-02** | Maximum 1,000 tracks per Playlist | Prevents UI slowdown. Encourages creating multiple focused playlists. |
| **NFR-RES-03** | History limited to 100 tracks (FIFO) | Sufficient for "recently played". Prevents unbounded growth. |
| **NFR-RES-04** | Max recursion depth = 10 levels | Prevents stack overflow from deep directories or symlink loops. |
| **NFR-RES-05** | Max path length = 4096 chars | Linux PATH_MAX. Prevents buffer issues. |

**Implementation Impact**:
```cpp
class Library {
private:
    static constexpr size_t MAX_FILES = 10000;
    
public:
    bool addMedia(std::shared_ptr<MediaFile> file) {
        if (mediaFiles_.size() >= MAX_FILES) {
            lastError_ = "Library full (max 10,000 files)";
            return false;
        }
        mediaFiles_.push_back(file);
        return true;
    }
};

class Playlist {
private:
    static constexpr size_t MAX_TRACKS = 1000;
    
public:
    bool addTrack(std::shared_ptr<MediaFile> track) {
        if (tracks_.size() >= MAX_TRACKS) {
            lastError_ = "Playlist full (max 1,000 tracks)";
            return false;
        }
        tracks_.push_back(track);
        return true;
    }
};

class History {
private:
    static constexpr size_t MAX_SIZE = 100;
    
public:
    void addTrack(std::shared_ptr<MediaFile> track) {
        history_.push_back(track);
        
        // FIFO removal
        while (history_.size() > MAX_SIZE) {
            history_.erase(history_.begin());
        }
    }
};
```

---

## Impact Summary

| Item | Lines Changed | Impact Level |
|------|--------------|--------------|
| FR-LIB-05 clarification | 1 line | Medium - Affects removal flow |
| FR-PLL-09 clarification | 1 line | High - Affects playlist management |
| FR-HWI-05 clarification | 1 line | High - Affects hardware sync logic |
| NFR-RES section added | 5 new requirements | Medium - Defines boundaries |

---

## Testing Implications

### New Test Cases Required:

1. **Library Removal Flow**:
   - Test removal with file in 0 playlists ✓
   - Test removal with file in 1 playlist ✓
   - Test removal with file in multiple playlists ✓
   - Test cancellation preserves state ✓

2. **Now Playing Protection**:
   - Verify delete button disabled ✓
   - Verify rename option hidden ✓
   - Test add/remove tracks still works ✓

3. **Hardware Sync**:
   - Test ADC debouncing (< 5% ignored) ✓
   - Test GUI priority window (2 seconds) ✓
   - Test button immediate processing ✓

4. **Resource Limits**:
   - Test each limit boundary ✓
   - Test error messages ✓
   - Test graceful degradation ✓

---

## Document Version

- **SRS Version**: v1.0 (updated)
- **Update Date**: 2026-02-02
- **Changes**: 4 clarifications added
- **Backward Compatibility**: Clarifications only, no breaking changes

---

## Summary

✅ **3 ambiguous requirements clarified**  
✅ **1 new section added (Resource Limits)**  
✅ **0 breaking changes** - Only clarifications  
✅ **Ready for implementation** - All conflicts resolved

**SRS is now complete and unambiguous!** 🎉
