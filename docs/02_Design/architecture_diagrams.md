# Music Player Architecture Diagrams

> [!IMPORTANT]
> **Corrected & Enhanced Architecture** - These diagrams reflect ALL critical fixes:
> 
> **Applied Fixes:**
> - ✅ `PlaybackStatus` enum (renamed from `PlaybackState` to avoid recursive type)
> - ✅ `PlaybackState` has `playQueue_` + `queueIndex_` for Next track functionality
> - ✅ `PlaybackController` has `IHardwareInterface* hardware_` for LCD write access
> - ✅ `MediaType` enum added (AUDIO/VIDEO/IMAGE/UNKNOWN) for type safety
> - ✅ `Subject` is thread-safe with `std::mutex mutex_` protection
> - ✅ `MediaMetadata` enhanced (sampleRate, channels, hasAlbumArt, MediaType)
> 
> See: [Critical Fixes](file:///wsl.localhost/Ubuntu/home/duong/music_player/docs/02_Design/critical_fixes.md) | [Review](file:///wsl.localhost/Ubuntu/home/duong/music_player/docs/02_Design/architecture_review_final.md)

## 1. Component Architecture Diagram

```mermaid
graph TB
    subgraph "Presentation Layer - ImGui Views"
        MainWindow[MainWindow]
        LibraryView[LibraryView]
        PlaylistView[PlaylistView]
        NowPlayingView[NowPlayingView]
        HistoryView[HistoryView]
        FileBrowserView[FileBrowserView]
    end
    
    subgraph "Business Logic Layer - Controllers"
        LibraryCtrl[LibraryController]
        PlaylistCtrl[PlaylistController]
        PlaybackCtrl[PlaybackController]
        HistoryCtrl[HistoryController]
        USBCtrl[USBController]
    end
    
    subgraph "Domain Model Layer"
        Library[Library]
        PlaylistMgr[PlaylistManager]
        Playlist[Playlist]
        History[History]
        PlaybackState[PlaybackState]
        MediaFile[MediaFile]
    end
    
    subgraph "Service Layer - Interfaces (DIP)"
        IPlaybackEngine[IPlaybackEngine]
        IFileSystem[IFileSystem]
        IMetadataReader[IMetadataReader]
        IHardwareInterface[IHardwareInterface]
        IPersistence[IPersistence]
    end
    
    subgraph "Infrastructure Layer - Implementations"
        SDL2Engine[SDL2PlaybackEngine]
        LocalFS[LocalFileSystem]
        TagLibReader[TagLibMetadataReader]
        S32K144[S32K144Interface]
        JsonPersist[JsonPersistence]
    end
    
    subgraph "Hardware"
        S32K144Board[S32K144 Board<br/>UART/ADC/Buttons]
        USB[USB Devices]
    end
    
    MainWindow --> LibraryView
    MainWindow --> PlaylistView
    MainWindow --> NowPlayingView
    MainWindow --> HistoryView
    MainWindow --> FileBrowserView
    
    LibraryView --> LibraryCtrl
    PlaylistView --> PlaylistCtrl
    NowPlayingView --> PlaybackCtrl
    HistoryView --> HistoryCtrl
    FileBrowserView --> LibraryCtrl
    
    LibraryCtrl --> Library
    LibraryCtrl --> IFileSystem
    LibraryCtrl --> IMetadataReader
    
    PlaylistCtrl --> PlaylistMgr
    PlaylistCtrl --> Library
    
    PlaybackCtrl --> IPlaybackEngine
    PlaybackCtrl --> PlaybackState
    PlaybackCtrl --> History
    
    HistoryCtrl --> History
    
    USBCtrl --> IFileSystem
    
    PlaylistMgr --> Playlist
    Library --> MediaFile
    Playlist --> MediaFile
    History --> MediaFile
    
    Library --> IPersistence
    PlaylistMgr --> IPersistence
    History --> IPersistence
    
    IPlaybackEngine -.implemented by.-> SDL2Engine
    IFileSystem -.implemented by.-> LocalFS
    IMetadataReader -.implemented by.-> TagLibReader
    IHardwareInterface -.implemented by.-> S32K144
    IPersistence -.implemented by.-> JsonPersist
    
    S32K144 --> S32K144Board
    LocalFS --> USB
    
    LibraryView -.observes.-> Library
    PlaylistView -.observes.-> PlaylistMgr
    NowPlayingView -.observes.-> PlaybackState
    HistoryView -.observes.-> History
    PlaybackCtrl -.observes.-> S32K144
    
    style MainWindow fill:#e1f5ff
    style LibraryView fill:#e1f5ff
    style PlaylistView fill:#e1f5ff
    style NowPlayingView fill:#e1f5ff
    style HistoryView fill:#e1f5ff
    style FileBrowserView fill:#e1f5ff
    
    style LibraryCtrl fill:#fff4e1
    style PlaylistCtrl fill:#fff4e1
    style PlaybackCtrl fill:#fff4e1
    style HistoryCtrl fill:#fff4e1
    style USBCtrl fill:#fff4e1
    
    style Library fill:#e8f5e9
    style PlaylistMgr fill:#e8f5e9
    style Playlist fill:#e8f5e9
    style History fill:#e8f5e9
    style PlaybackState fill:#e8f5e9
```

# New Class Diagram Content

Replace section 2 "Class Diagram - Core Relationships" in architecture_diagrams.md from line 140 to line 426 with this:

```mermaid
classDiagram

    %% ========================================
    %% OBSERVER PATTERN FOUNDATION
    %% ========================================
    class IObserver {
        <<interface>>
        +update(subject)*
    }
   
    class ISubject {
        <<interface>>
        +attach(observer)*
        +detach(observer)*
        +notify()*
    }
   
    class Subject {
        <<abstract>>
        #observers: vector~IObserver*~
        #observerMutex: mutex
        +attach(observer)
        +detach(observer)
        +notify()
    }
   
    %% ========================================
    %% ENUMERATIONS & VALUE OBJECTS
    %% ========================================
    class PlaybackStatus {
        <<enumeration>>
        STOPPED
        PLAYING
        PAUSED
    }
   
    class MediaType {
        <<enumeration>>
        AUDIO
        VIDEO
        IMAGE
        UNKNOWN
    }
   
    class MediaMetadata {
        <<value object>>
        +title: string
        +artist: string
        +album: string
        +duration: double
        +bitrate: int
        +sampleRate: int
        +channels: int
        +hasAlbumArt: bool
    }
   
    %% ========================================
    %% SERVICE LAYER - INTERFACES
    %% ========================================
    class IPlaybackEngine {
        <<interface>>
        +play(filepath) bool*
        +pause()*
        +resume()*
        +stop()*
        +seek(position)*
        +setVolume(volume)*
        +getState() PlaybackStatus*
        +getPosition() double*
        +getDuration() double*
    }
   
    class IFileSystem {
        <<interface>>
        +browse(path) vector~string~*
        +scanDirectory(path, extensions) vector~string~*
        +detectUSBDevices() vector~string~*
        +mountUSB(device, mountPoint) bool*
        +unmountUSB(mountPoint) bool*
        +fileExists(path) bool*
    }
   
    class IMetadataReader {
        <<interface>>
        +readMetadata(filepath) MediaMetadata*
        +writeMetadata(filepath, metadata) bool*
        +canWrite(filepath) bool*
    }
   
    class IHardwareInterface {
        <<interface>>
        +initialize(port, baudRate) bool*
        +sendCommand(command)*
        +readData() string*
        +displayText(text)*
        +readADC() int*
        +isConnected() bool*
    }
   
    class IPersistence {
        <<interface>>
        +saveToFile(filepath, data) bool*
        +loadFromFile(filepath) string*
        +fileExists(filepath) bool*
    }
   
    %% ========================================
    %% SERVICE LAYER - IMPLEMENTATIONS
    %% ========================================
    class SDL2PlaybackEngine {
        -playbackThread: thread
        -status: PlaybackStatus
        -dataMutex: mutex
        -audioSpec: SDL_AudioSpec
        -currentPosition: double
        -totalDuration: double
        +play(filepath) bool
        +pause()
        +resume()
        +stop()
        +getState() PlaybackStatus
        -playbackLoop()
    }
   
    class LocalFileSystem {
        -allowedPaths: vector~string~
        +browse(path) vector~string~
        +scanDirectory(path, extensions) vector~string~
        +detectUSBDevices() vector~string~
        +mountUSB(device, mountPoint) bool
        -isPathAllowed(path) bool
    }
   
    class TagLibMetadataReader {
        +readMetadata(filepath) MediaMetadata
        +writeMetadata(filepath, metadata) bool
        +canWrite(filepath) bool
        -extractAudioMetadata(file) MediaMetadata
        -extractVideoMetadata(file) MediaMetadata
    }
   
    class S32K144Interface {
        -serialFd: int
        -listenerThread: thread
        -running: atomic~bool~
        -lastCommand: string
        -dataMutex: mutex
        +initialize(port, baudRate) bool
        +sendCommand(command)
        +displayText(text)
        +readADC() int
        -startListening()
        -parseData(data)
    }
   
    class JsonPersistence {
        +saveToFile(filepath, data) bool
        +loadFromFile(filepath) string
        +fileExists(filepath) bool
        -validateJson(data) bool
    }
   
    %% ========================================
    %% MODEL LAYER - DOMAIN ENTITIES
    %% ========================================
    class MediaFile {
        <<entity>>
        -filepath: string
        -metadata: MediaMetadata
        -type: MediaType
        -inLibrary: bool
        -addedDate: time_t
        +getDisplayName() string
        +getFilepath() string
        +getMetadata() MediaMetadata
        +isInLibrary() bool
        +setInLibrary(flag)
    }
   
    class Library {
        <<aggregate root>>
        -mediaFiles: vector~MediaFile~
        -persistence: IPersistence*
        -dataMutex: mutex
        -filepath: string
        +addMedia(file)
        +removeMedia(filepath)
        +search(query) vector~MediaFile~
        +getAll() vector~MediaFile~
        +contains(filepath) bool
        +save()
        +load()
    }
   
    class Playlist {
        <<entity>>
        -name: string
        -tracks: vector~MediaFile~
        -loopEnabled: bool
        -shuffleEnabled: bool
        -dataMutex: mutex
        +addTrack(track)
        +removeTrack(index)
        +getTrack(index) MediaFile
        +shuffle()
        +getTracks() vector~MediaFile~
        +setLoop(enabled)
    }
   
    class PlaylistManager {
        <<aggregate root>>
        -playlists: map~string, Playlist~
        -persistence: IPersistence*
        -dataMutex: mutex
        -playlistDir: string
        +createPlaylist(name) Playlist*
        +deletePlaylist(name) bool
        +getPlaylist(name) Playlist*
        +getAllPlaylists() vector~string~
        +save()
        +load()
    }
   
    class History {
        <<entity>>
        -history: vector~MediaFile~
        -maxSize: size_t
        -dataMutex: mutex
        -persistence: IPersistence*
        +addTrack(track)
        +removeTrack(index)
        +clear()
        +getHistory() vector~MediaFile~
        +save()
        +load()
    }
   
    class PlaybackState {
        <<aggregate root>>
        -currentTrack: MediaFile*
        -status: PlaybackStatus
        -volume: float
        -position: double
        -backStack: stack~MediaFile~
        -playQueue: vector~MediaFile~
        -queueIndex: size_t
        -dataMutex: mutex
        +setCurrentTrack(track)
        +getCurrentTrack() MediaFile*
        +setStatus(status)
        +getStatus() PlaybackStatus
        +setVolume(volume)
        +getVolume() float
        +setPosition(position)
        +getPosition() double
        +pushToBackStack()
        +popFromBackStack() MediaFile*
        +setPlayQueue(queue)
        +getNextTrack() MediaFile*
        +getPreviousTrack() MediaFile*
        +hasNextTrack() bool
    }
   
    %% ========================================
    %% CONTROLLER LAYER
    %% ========================================
    class LibraryController {
        -library: Library*
        -fileSystem: IFileSystem*
        -metadataReader: IMetadataReader*
        +addMediaFilesFromDirectory(path)
        +addMediaFile(filepath)
        +removeMediaFile(filepath)
        +searchMedia(query) vector~MediaFile~
        +getAllMedia() vector~MediaFile~
        +saveLibrary()
        +loadLibrary()
    }
   
    class PlaylistController {
        -playlistManager: PlaylistManager*
        -library: Library*
        +createPlaylist(name)
        +deletePlaylist(name)
        +renamePlaylist(oldName, newName)
        +addTrackToPlaylist(playlist, track)
        +removeTrackFromPlaylist(playlist, index)
        +getPlaylist(name) Playlist*
        +getAllPlaylists() vector~string~
        +savePlaylists()
        +loadPlaylists()
    }
   
    class PlaybackController {
        -engine: IPlaybackEngine*
        -state: PlaybackState*
        -history: History*
        -hardware: IHardwareInterface*
        -currentPlaylist: Playlist*
        +play(track)
        +pause()
        +resume()
        +stop()
        +next()
        +previous()
        +seek(position)
        +setVolume(volume)
        +setPlaylist(playlist)
        +update(subject)
        -sendMetadataToHardware(track)
        -handleHardwareEvent()
    }
   
    class HistoryController {
        -history: History*
        +addToHistory(track)
        +removeFromHistory(index)
        +clearHistory()
        +getHistory() vector~MediaFile~
        +saveHistory()
        +loadHistory()
    }
   
    class USBController {
        -fileSystem: IFileSystem*
        -monitorThread: thread
        -running: atomic~bool~
        -mountedDevices: map~string,string~
        +startMonitoring()
        +stopMonitoring()
        +getMountedDevices() vector~string~
        +update(subject)
        -monitorLoop()
        -handleDeviceAdded(device)
        -handleDeviceRemoved(device)
    }
   
    %% ========================================
    %% VIEW LAYER
    %% ========================================
    class IView {
        <<interface>>
        +render()*
        +handleInput()*
        +update(subject)*
        +show()*
        +hide()*
    }
   
    class IViewFactory {
        <<interface>>
        +createMainWindow() IView*
        +createLibraryView(controller, library) IView*
        +createPlaylistView(controller, manager) IView*
        +createNowPlayingView(controller, state) IView*
        +createHistoryView(controller, history) IView*
        +createFileBrowserView(fileSystem, controller) IView*
    }
    
    class ViewFactory {
        +createMainWindow() IView*
        +createLibraryView(controller, library) IView*
        +createPlaylistView(controller, manager) IView*
        +createNowPlayingView(controller, state) IView*
        +createHistoryView(controller, history) IView*
        +createFileBrowserView(fileSystem, controller) IView*
    }
   
    class BaseView {
        <<abstract>>
        #visible: bool
        +render()*
        +handleInput()
        +update(subject)
        +show()
        +hide()
        #renderImpl()*
    }
   
    class LibraryView {
        -controller: LibraryController*
        -library: Library*
        -selectedIndex: int
        -searchQuery: string
        +render()
        +update(subject)
        -renderSearchBar()
        -renderMediaList()
        -handleAddToPlaylist()
    }
   
    class PlaylistView {
        -controller: PlaylistController*
        -playlistManager: PlaylistManager*
        -selectedPlaylist: string
        -selectedTrack: int
        +render()
        +update(subject)
        -renderPlaylistSelector()
        -renderTrackList()
        -handleCreatePlaylist()
    }
   
    class NowPlayingView {
        -controller: PlaybackController*
        -state: PlaybackState*
        +render()
        +update(subject)
        -renderProgressBar()
        -renderControls()
        -renderMetadata()
    }
   
    class HistoryView {
        -controller: HistoryController*
        -history: History*
        -selectedIndex: int
        +render()
        +update(subject)
        -renderHistoryList()
        -handlePlayFromHistory()
    }
   
    class FileBrowserView {
        -controller: LibraryController*
        -usbController: USBController*
        -currentPath: string
        -selectedFiles: vector~int~
        +render()
        +update(subject)
        -renderPathNavigator()
        -renderFileList()
        -renderUSBSection()
    }
   
    class MainWindow {
        -views: map~string,IView*~
        -activeView: string
        +render()
        +switchView(name)
        -renderMenuBar()
        -renderStatusBar()
    }
   
    %% ========================================
    %% INHERITANCE RELATIONSHIPS
    %% ========================================
    ISubject <|-- Subject : implements
   
    Subject <|-- Library : extends
    Subject <|-- PlaylistManager : extends
    Subject <|-- Playlist : extends
    Subject <|-- History : extends
    Subject <|-- PlaybackState : extends
    Subject <|-- SDL2PlaybackEngine : extends
    Subject <|-- S32K144Interface : extends
   
    IObserver <|.. IView : implements
    IView <|.. BaseView : implements
   
    BaseView <|-- LibraryView : extends
    BaseView <|-- PlaylistView : extends
    BaseView <|-- NowPlayingView : extends
    BaseView <|-- HistoryView : extends
    BaseView <|-- FileBrowserView : extends
   
    IObserver <|.. PlaybackController : implements
    IObserver <|.. USBController : implements
   
    IPlaybackEngine <|.. SDL2PlaybackEngine : implements
    IFileSystem <|.. LocalFileSystem : implements
    IMetadataReader <|.. TagLibMetadataReader : implements
    IHardwareInterface <|.. S32K144Interface : implements
    IPersistence <|.. JsonPersistence : implements
    
    IViewFactory <|.. ViewFactory : implements
   
    %% ========================================
    %% COMPOSITION - MODEL LAYER
    %% ========================================
    Library *-- MediaFile : contains
    Playlist *-- MediaFile : contains
    History *-- MediaFile : contains
    PlaylistManager *-- Playlist : manages
   
    MediaFile *-- MediaMetadata : has
    MediaFile --> MediaType : uses
    PlaybackState --> PlaybackStatus : uses
    PlaybackState --> MediaFile : references
   
    %% ========================================
    %% DEPENDENCIES - MODEL TO SERVICE
    %% ========================================
    Library --> IPersistence : uses
    PlaylistManager --> IPersistence : uses
    History --> IPersistence : uses
   
    %% ========================================
    %% DEPENDENCIES - CONTROLLER TO MODEL
    %% ========================================
    LibraryController --> Library : manages
   
    PlaylistController --> PlaylistManager : manages
    PlaylistController --> Library : queries
   
    PlaybackController --> PlaybackState : manages
    PlaybackController --> History : updates
    PlaybackController --> Playlist : uses
   
    HistoryController --> History : manages
   
    %% ========================================
    %% DEPENDENCIES - CONTROLLER TO SERVICE
    %% ========================================
    LibraryController --> IFileSystem : uses
    LibraryController --> IMetadataReader : uses
   
    PlaybackController --> IPlaybackEngine : uses
    PlaybackController --> IHardwareInterface : uses
   
    USBController --> IFileSystem : uses
   
    %% ========================================
    %% DEPENDENCIES - VIEW TO CONTROLLER
    %% ========================================
    LibraryView --> LibraryController : calls
    PlaylistView --> PlaylistController : calls
    NowPlayingView --> PlaybackController : calls
    HistoryView --> HistoryController : calls
   
    FileBrowserView --> LibraryController : calls
    FileBrowserView --> USBController : calls
   
    MainWindow --> IView : manages
    ViewFactory --> IView : returns
   
    %% ========================================
    %% OBSERVER PATTERN - VIEW TO MODEL
    %% ========================================
    Library --o LibraryView : notifies
    PlaylistManager --o PlaylistView : notifies
    PlaybackState --o NowPlayingView : notifies
    History --o HistoryView : notifies
   
    %% ========================================
    %% OBSERVER PATTERN - CONTROLLER TO SERVICE
    %% ========================================
    SDL2PlaybackEngine --o PlaybackController : notifies
    S32K144Interface --o PlaybackController : notifies
   
    %% ========================================
    %% STYLING
    %% ========================================
   
    %% Observer Foundation
    style IObserver fill:#e1bee7,stroke:#333,stroke-width:2px
    style ISubject fill:#e1bee7,stroke:#333,stroke-width:2px
    style Subject fill:#ce93d8,stroke:#333,stroke-width:2px
   
    %% Value Objects
    style PlaybackStatus fill:#e0e0e0,stroke:#333,stroke-width:2px
    style MediaType fill:#e0e0e0,stroke:#333,stroke-width:2px
    style MediaMetadata fill:#eeeeee,stroke:#333,stroke-width:2px
   
    %% Service Interfaces
    style IPlaybackEngine fill:#bbdefb,stroke:#333,stroke-width:2px
    style IFileSystem fill:#bbdefb,stroke:#333,stroke-width:2px
    style IMetadataReader fill:#bbdefb,stroke:#333,stroke-width:2px
    style IHardwareInterface fill:#bbdefb,stroke:#333,stroke-width:2px
    style IPersistence fill:#bbdefb,stroke:#333,stroke-width:2px
   
    %% Service Implementations
    style SDL2PlaybackEngine fill:#90caf9,stroke:#333,stroke-width:2px
    style LocalFileSystem fill:#90caf9,stroke:#333,stroke-width:2px
    style TagLibMetadataReader fill:#90caf9,stroke:#333,stroke-width:2px
    style S32K144Interface fill:#90caf9,stroke:#333,stroke-width:2px
    style JsonPersistence fill:#90caf9,stroke:#333,stroke-width:2px
   
    %% Model Layer
    style MediaFile fill:#c8e6c9,stroke:#333,stroke-width:2px
    style Library fill:#a5d6a7,stroke:#333,stroke-width:2px
    style Playlist fill:#a5d6a7,stroke:#333,stroke-width:2px
    style PlaylistManager fill:#81c784,stroke:#333,stroke-width:2px
    style History fill:#a5d6a7,stroke:#333,stroke-width:2px
    style PlaybackState fill:#81c784,stroke:#333,stroke-width:2px
   
    %% Controller Layer
    style LibraryController fill:#ffcc80,stroke:#333,stroke-width:2px
    style PlaylistController fill:#ffcc80,stroke:#333,stroke-width:2px
    style PlaybackController fill:#ffcc80,stroke:#333,stroke-width:2px
    style HistoryController fill:#ffcc80,stroke:#333,stroke-width:2px
    style USBController fill:#ffcc80,stroke:#333,stroke-width:2px
   
    %% View Layer
    style IView fill:#ffccbc,stroke:#333,stroke-width:2px
    style IViewFactory fill:#ffccbc,stroke:#333,stroke-width:2px
    style ViewFactory fill:#ffab91,stroke:#333,stroke-width:2px
    style BaseView fill:#ffab91,stroke:#333,stroke-width:2px
    style LibraryView fill:#ff8a65,stroke:#333,stroke-width:2px
    style PlaylistView fill:#ff8a65,stroke:#333,stroke-width:2px
    style NowPlayingView fill:#ff8a65,stroke:#333,stroke-width:2px
    style HistoryView fill:#ff8a65,stroke:#333,stroke-width:2px
    style FileBrowserView fill:#ff8a65,stroke:#333,stroke-width:2px
    style MainWindow fill:#ff7043,stroke:#333,stroke-width:2px
```


## 3. Sequence Diagram - Play Song Flow

```mermaid
sequenceDiagram
    actor User
    participant NowPlayingView
    participant PlaybackController
    participant IPlaybackEngine
    participant PlaybackState
    participant History
    participant SDL2PlaybackEngine
    
    User->>NowPlayingView: Click Play Button
    NowPlayingView->>PlaybackController: play(mediaFile)
    
    activate PlaybackController
    PlaybackController->>History: addTrack(mediaFile)
    History->>History: notify()
    
    PlaybackController->>PlaybackState: setCurrentTrack(mediaFile)
    PlaybackState->>PlaybackState: notify()
    
    PlaybackController->>IPlaybackEngine: play(filepath)
    activate IPlaybackEngine
    IPlaybackEngine->>SDL2PlaybackEngine: play(filepath)
    SDL2PlaybackEngine->>SDL2PlaybackEngine: Start playback thread
    SDL2PlaybackEngine->>SDL2PlaybackEngine: notify()
    IPlaybackEngine-->>PlaybackController: true
    deactivate IPlaybackEngine
    deactivate PlaybackController
    
    Note over SDL2PlaybackEngine: Playback thread running
    
    SDL2PlaybackEngine->>PlaybackState: Update position/duration
    PlaybackState->>PlaybackState: notify()
    PlaybackState->>NowPlayingView: update(subject)
    NowPlayingView->>NowPlayingView: Refresh UI
    
    Note over NowPlayingView: UI shows progress bar,<br/>current time, metadata
```

## 4. Sequence Diagram - Hardware Control Flow

```mermaid
sequenceDiagram
    actor HardwareUser
    participant S32K144Board
    participant S32K144Interface
    participant PlaybackController
    participant IPlaybackEngine
    participant PlaybackState
    participant NowPlayingView
    
    HardwareUser->>S32K144Board: Press "Next" Button
    
    Note over S32K144Interface: Listener thread running
    
    S32K144Board->>S32K144Interface: UART: "CMD:NEXT"
    S32K144Interface->>S32K144Interface: parseData()
    S32K144Interface->>S32K144Interface: notify()
    
    S32K144Interface->>PlaybackController: update(subject)
    activate PlaybackController
    PlaybackController->>S32K144Interface: getLastEvent()
    S32K144Interface-->>PlaybackController: HardwareEvent(NEXT)
    
    PlaybackController->>PlaybackController: next()
    PlaybackController->>IPlaybackEngine: play(nextTrack)
    IPlaybackEngine-->>PlaybackController: OK
    
    PlaybackController->>PlaybackState: setCurrentTrack(nextTrack)
    PlaybackState->>PlaybackState: notify()
    deactivate PlaybackController
    
    PlaybackState->>NowPlayingView: update(subject)
    NowPlayingView->>NowPlayingView: Refresh UI
    
    Note over NowPlayingView: Shows new track info
```

## 5. Sequence Diagram - USB Hotplug Detection

```mermaid
sequenceDiagram
    participant USBDevice
    participant USBController
    participant IFileSystem
    participant FileBrowserView
    participant LibraryController
    
    Note over USBController: Monitoring thread running
    
    USBDevice->>USBController: USB Connected
    USBController->>IFileSystem: detectUSBDevices()
    IFileSystem-->>USBController: ["/dev/sdb1"]
    
    USBController->>IFileSystem: mountUSB("/dev/sdb1", "/mnt/usb")
    IFileSystem-->>USBController: true
    
    USBController->>USBController: notify()
    USBController->>FileBrowserView: update(subject)
    
    FileBrowserView->>FileBrowserView: Refresh USB section
    
    Note over FileBrowserView: User sees USB device
    
    FileBrowserView->>IFileSystem: scanDirectory("/mnt/usb", [".mp3", ".mp4"])
    IFileSystem-->>FileBrowserView: [file1.mp3, file2.mp4]
    
    FileBrowserView->>FileBrowserView: Display files
    
    Note over FileBrowserView: User can add to library
```

## 6. State Machine Diagram - Playback States

```mermaid
stateDiagram-v2
    [*] --> STOPPED
    
    STOPPED --> PLAYING: play(file)
    PLAYING --> PAUSED: pause()
    PAUSED --> PLAYING: resume()
    PLAYING --> STOPPED: stop()
    PAUSED --> STOPPED: stop()
    
    PLAYING --> PLAYING: next() / previous()
    PLAYING --> PLAYING: seek(position)
    PAUSED --> PAUSED: seek(position)
    
    PLAYING --> STOPPED: playback finished<br/>[no loop, no next track]
    PLAYING --> PLAYING: playback finished<br/>[loop enabled OR has next]
    
    state PLAYING {
        [*] --> Streaming
        Streaming --> Seeking: seek()
        Seeking --> Streaming: seek complete
        
        Streaming --> UpdatingPosition: Every frame
        UpdatingPosition --> Streaming
    }
    
    note right of STOPPED
        - currentTrack = null
        - position = 0
        - No audio output
    end note
    
    note right of PLAYING
        - Audio streaming
        - Position updating
        - Notify observers
    end note
    
    note right of PAUSED
        - Audio paused
        - Position preserved
        - Can seek
    end note
```

## 7. Observer Pattern - Data Flow Diagram

```mermaid
graph LR
    subgraph "Subjects (Models)"
        Library[Library<br/>Subject]
        PlaylistMgr[PlaylistManager<br/>Subject]
        History[History<br/>Subject]
        PlaybackState[PlaybackState<br/>Subject]
    end
    
    subgraph "Observers (Views)"
        LibraryView[LibraryView<br/>Observer]
        PlaylistView[PlaylistView<br/>Observer]
        HistoryView[HistoryView<br/>Observer]
        NowPlayingView[NowPlayingView<br/>Observer]
    end
    
    subgraph "Special Observers"
        PlaybackCtrl[PlaybackController<br/>Observer]
        S32K144[S32K144Interface<br/>Subject]
    end
    
    Library -->|notify| LibraryView
    PlaylistMgr -->|notify| PlaylistView
    History -->|notify| HistoryView
    PlaybackState -->|notify| NowPlayingView
    
    S32K144 -->|notify| PlaybackCtrl
    
    LibraryView -.->|attach| Library
    PlaylistView -.->|attach| PlaylistMgr
    HistoryView -.->|attach| History
    NowPlayingView -.->|attach| PlaybackState
    PlaybackCtrl -.->|attach| S32K144
    
    style Library fill:#90ee90
    style PlaylistMgr fill:#90ee90
    style History fill:#90ee90
    style PlaybackState fill:#90ee90
    style S32K144 fill:#90ee90
    
    style LibraryView fill:#87ceeb
    style PlaylistView fill:#87ceeb
    style HistoryView fill:#87ceeb
    style NowPlayingView fill:#87ceeb
    style PlaybackCtrl fill:#ffa07a
```

## 8. Dependency Injection Container - Application Class

```mermaid
graph TB
    App[Application<br/>DI Container]
    
    subgraph "Creates Services"
        App --> SDL2[SDL2PlaybackEngine]
        App --> LocalFS[LocalFileSystem]
        App --> TagLib[TagLibMetadataReader]
        App --> S32K[S32K144Interface]
        App --> Json[JsonPersistence]
    end
    
    subgraph "Creates Models"
        App --> Library[Library]
        App --> PlaylistMgr[PlaylistManager]
        App --> History[History]
        App --> PBState[PlaybackState]
    end
    
    subgraph "Creates Controllers"
        App --> LibCtrl[LibraryController]
        App --> PlCtrl[PlaylistController]
        App --> PBCtrl[PlaybackController]
        App --> HistCtrl[HistoryController]
        App --> USBCtrl[USBController]
    end
    
    subgraph "Creates Views"
        App --> MainWin[MainWindow]
        App --> LibView[LibraryView]
        App --> PlView[PlaylistView]
        App --> NPView[NowPlayingView]
        App --> HistView[HistoryView]
        App --> FBView[FileBrowserView]
    end
    
    subgraph "Injects Dependencies"
        LibCtrl -.->|IFileSystem*| LocalFS
        LibCtrl -.->|IMetadataReader*| TagLib
        LibCtrl -.->|Library*| Library
        
        PBCtrl -.->|IPlaybackEngine*| SDL2
        PBCtrl -.->|PlaybackState*| PBState
        PBCtrl -.->|History*| History
        
        LibView -.->|LibraryController*| LibCtrl
        LibView -.->|Library*| Library
        
        NPView -.->|PlaybackController*| PBCtrl
        NPView -.->|PlaybackState*| PBState
    end
    
    subgraph "Wires Observers"
        Library -.->|attach| LibView
        PBState -.->|attach| NPView
        S32K -.->|attach| PBCtrl
    end
    
    style App fill:#ff6b6b
    style SDL2 fill:#51cf66
    style LocalFS fill:#51cf66
    style LibCtrl fill:#ffd43b
    style LibView fill:#74c0fc
```

## 9. MVC Pattern Implementation

```mermaid
graph TB
    subgraph "View Layer - ImGui"
        Views[Views<br/>LibraryView<br/>PlaylistView<br/>NowPlayingView<br/>HistoryView]
    end
    
    subgraph "Controller Layer"
        Controllers[Controllers<br/>LibraryController<br/>PlaylistController<br/>PlaybackController<br/>HistoryController]
    end
    
    subgraph "Model Layer"
        Models[Models<br/>Library<br/>PlaylistManager<br/>History<br/>PlaybackState]
    end
    
    User[User Input<br/>ImGui Events]
    
    User -->|1. Interaction| Views
    Views -->|2. Call Methods| Controllers
    Controllers -->|3. Update State| Models
    Models -.->|4. Notify| Views
    Views -->|5. Re-render| User
    
    Controllers -->|Query Data| Models
    
    style User fill:#e1f5ff
    style Views fill:#e1f5ff
    style Controllers fill:#fff4e1
    style Models fill:#e8f5e9
```

## Usage Notes

- **Component Diagram**: Shows overall architecture with all layers
- **Class Diagram**: Shows key classes and their relationships
- **Sequence Diagrams**: Show runtime behavior for key use cases
- **State Machine**: Shows PlaybackState transitions
- **Observer Pattern**: Shows notification flow
- **DI Container**: Shows how Application wires everything together
- **MVC**: Shows the MVC pattern implementation

You can copy any of these Mermaid diagrams into:
- GitHub markdown files (rendered automatically)
- Mermaid Live Editor: https://mermaid.live/
- Documentation tools that support Mermaid
- PlantUML converters if you need UML format
