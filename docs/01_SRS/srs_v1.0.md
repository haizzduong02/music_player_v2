# Software Requirements Specification (SRS)

## 1. Introduction

### 1.1 Purpose
Tài liệu này đặc tả các yêu cầu phần mềm cho Media Player Application, bao gồm yêu cầu chức năng, phi chức năng, use case và các ràng buộc hệ thống nhằm định hướng thiết kế, phát triển và kiểm thử.

### 1.2 Scope
Ứng dụng là Media Player hỗ trợ GUI, cung cấp các tính năng:
- Duyệt file media từ Local Storage và thiết bị USB.
- Lưu trữ các file media đã duyệt dưới dạng Metadata và lưu vào thư viện.
- Quản lí playlist (Create, Read, Update, Delete).
- Quản lí lịch sử phát.
- Phát audio/video sử dụng thư viện SDL2.
- Điều khiển Playback thông qua phần cứng (board S32K144).

### 1.3 Definitions and Acronyms
| Term | Definition |
| :--- | :--- |
| CLI | Command Line Interface |
| GUI | Graphical User Interface |
| MVC | Model-View-Controller Architecture |
| TagLib | Thư viện đọc và chỉnh sửa Audio Metadata |
| SDL2 | Simple DirectMedia Layer 2 Library |
| S32K144 | Vi điều khiển NXP dùng để điều khiển phần cứng |
| ADC | Analog-to-Digital Converter |
| UART | Universal Asynchronous Receiver-Transmitter |

### 1.4 References
- [System requirement](./../00_Requirement/req.txt)
- [TagLib](https://taglib.org/)
- [SDL2](https://www.libsdl.org/)
- [S32K144](https://www.nxp.com/products/microcontrollers/s32k-automotive-mcus/s32k1-general-purpose-mcus/s32k144-32-bit-arm-cortex-m4-based-automotive-mcu:S32K144)

---

## 2. Overall Description

### 2.1 Product Perspective
Ứng dụng hoạt động độc lập trên Linux, tương tác với các thành phần:
1. **Local File System**: Truy xuất file media.
2. **USB Storage**: Truy xuất media từ bộ nhớ ngoài.
3. **Audio Subsystem**: Phát playback qua SDL2.
4. **GUI**: Sử dụng thư viện ImGui để hiển thị giao diện. 
5. **S32K144 Board**: Điều khiển ngoại vi qua Serial/UART.

### 2.2 Product Features Summary
| Feature | Description |
| :--- | :--- |
| File Browsing | Duyệt thư mục và liệt kê file media. |
| USB Support | Tự động phát hiện và Mount thiết bị USB. |
| Library | Lưu trữ các file media đã duyệt dưới dạng Metadata. |
| Playlist Management | Create, edit, rename, và delete playlist. |
| Metadata | Xem và chỉnh sửa Tags (Artist, Title...). |
| Playback | Điều khiển Play, Pause, Next, Previous, Loop, Volume. |
| History | Lưu lịch sử các file đã phát. |
| Hardware Control | Điều khiển qua nút bấm vật lý và biến trở trên S32K144. |

### 2.3 User Classes
| Class | Description |
| :--- | :--- |
| GUI User | Tương tác qua bàn phím và màn hình GUI. |
| Hardware User | Tương tác qua các nút bấm vật lý trên board được kết nối. |

### 2.4 Operating Environment
- **OS**: Linux (khuyến nghị Ubuntu 20.04+).
- **Language**: C++17 or newer.
- **Hardware**: PC có cổng USB; board S32K144.

### 2.5 Assumptions and Dependencies
- Thiết bị USB chứa các định dạng media được hỗ trợ.
- Board S32K144 được kết nối qua cổng serial tiêu chuẩn (ví dụ: `/dev/ttyUSB0`).

---

## 3. Functional Requirements

| ID | Requirement | Priority |
| :--- | :--- | :--- |
| FR-FIL | File System Navigation | High |
| FR-PLL | Playlist Management | High |
| FR-MDA | Metadata Management | High |
| FR-LIB | Library Management | High |
| FR-HIS | History Management | High |
| FR-AUD | Playback (SDL2) | High |
| FR-HWI | Hardware Interaction | High |

### 3.1 File System Navigation
| ID | Requirement | Priority |
| :--- | :--- | :--- |
| FR-FIL-01 | Duyệt file từ các thư mục Local. | High |
| FR-FIL-02 | Hỗ trợ Mount và đọc thiết bị USB. | High |
| FR-FIL-03 | Quét đệ quy thư mục con để tìm file media. | Medium |
| FR-FIL-04 | Lọc file theo định dạng hỗ trợ (.mp3, .mp4...). | Medium |

### 3.2 Metadata Management
| ID | Requirement | Priority |
| :--- | :--- | :--- |
| FR-MDA-01 | Hiển thị metadata Audio (Artist, Album, Title,...). | High |
| FR-MDA-02 | Hiển thị metadata Video (Title, Duration, Bitrate, Codec,...). | High |
| FR-MDA-03 | Cho phép **Edit** các thẻ metadata (nếu định dạng hỗ trợ). | Low |
| FR-MDA-04 | Ghi nhận thay đổi metadata vào file gốc. | Low |

### 3.3 Library Management
| ID | Requirement | Priority |
| :--- | :--- | :--- |
| FR-LIB-01 | Lưu trữ các file đã duyệt, được chọn dưới dạng Metadata | High |
| FR-LIB-02 | Cho phép **Add**/**Remove** file vào/ra thư viện. | High |
| FR-LIB-03 | Lưu trữ dữ liệu Library giữa các phiên làm việc. | Medium |
| FR-LIB-04 | Khi add file mới vào Playlist, tự động add vào Library. | High |
| FR-LIB-05 | Khi remove file ra khỏi Library, hệ thống hiển thị cảnh báo liệt kê các Playlist chứa file đó. User xác nhận để remove khỏi cả Library và tất cả Playlist, hoặc Cancel để giữ nguyên. File đã remove khỏi Library không thể play được từ Playlist. | High |
| FR-LIB-06 | Duyệt file đảm bảo gắn cờ file đã được add vào thư viện. | High |
| FR-LIB-07 | Cho phép tìm kiếm theo các trường khác nhau trong Library | High|

### 3.4 History Management
| ID | Requirement | Priority |
| :--- | :--- | :--- |
| FR-HIS-01 | Lưu `N` track đã phát gần nhất. | High |
| FR-HIS-02 | Hiển thị danh sách lịch sử phát. | High |
| FR-HIS-03 | Cho phép phát lại bài hát từ lịch sử. | High |
| FR-HIS-04 | Cho phép xóa một media khỏi lịch sử. | High |
| FR-HIS-05 | Tự động đưa track trùng lặp lên, tránh duplicate trong lịch sử. | Medium |
| FR-HIS-06 | Cho phép **Clear**/**Delete** lịch sử. | High |

### 3.5 Playlist Management
| ID | Requirement | Priority |
| :--- | :--- | :--- |
| FR-PLL-01 | Hiển thị danh sách playlist đã lưu. | High |
| FR-PLL-02 | Hiển thị nội dung của playlist được chọn. | High |
| FR-PLL-03 | Cho phép **Create** playlist mới. | High |
| FR-PLL-04 | Cho phép **Add**/**Remove** file khỏi playlist. | High |
| FR-PLL-05 | Cho phép **Rename** playlist. | Medium |
| FR-PLL-06 | Cho phép **Delete** playlist. | High |
| FR-PLL-07 | Cho phép **Loop**/**Shuffle** playlist. | High |
| FR-PLL-08 | Lưu trữ Playlist xuống ổ đĩa (Persist to disk). | Medium |
| FR-PLL-09 | Luôn có một Playlist mặc định có tên "Now Playing". Playlist này **không thể delete** (FR-PLL-06 không áp dụng cho "Now Playing"). User có thể Add/Remove tracks nhưng không thể Delete hoặc Rename playlist này. | High |
| FR-PLL-10 | Người dùng **Add** file, hệ thống điều hướng tới Library, từ Library có thể điều hướng để duyệt file local + USB. | High |

### 3.6 Playback (SDL2)
| ID | Requirement | Priority |
| :--- | :--- | :--- |
| FR-AUD-01 | Phát Audio/Video bằng thư viện SDL2. | High |
| FR-AUD-02 | Luồng Playback chạy trên Thread riêng biệt (Non-blocking). | High |
| FR-AUD-03 | Hỗ trợ các tác vụ: Play, Pause, Next, Previous, Loop. | High |
| FR-AUD-04 | Tự động phát bài tiếp theo trong Playlist hoặc Library. | High |
| FR-AUD-05 | Hiển thị thời gian hiện tại và tổng thời lượng track. | High |
| FR-AUD-06 | Điều chỉnh **Volume** bằng phần mềm. | High |
| FR-AUD-07 | Có một Playback Back Stack phục vụ cho Previous. | High |

### 3.7 Hardware Interaction
| ID | Requirement | Priority |
| :--- | :--- | :--- |
| FR-HWI-01 | Giao tiếp với board S32K144 qua giao thức UART. | High |
| FR-HWI-02 | Điều khiển Volume thông qua giá trị ADC từ board. | High |
| FR-HWI-03 | Điều khiển Playback (Play, Pause, Next) từ nút bấm trên board. | High |
| FR-HWI-04 | Gửi Title bài hát hiện tại xuống board để hiển thị LCD. | Low |
| FR-HWI-05 | Hệ thống ưu tiên giá trị tuyệt đối từ phần cứng khi có thay đổi. **Debouncing**: ADC volume changes < 5% trong 500ms bị bỏ qua để tránh noise. **Priority rule**: GUI volume change được ưu tiên trong 2 giây, sau đó hardware takes over. Button events (Play/Pause/Next) được xử lý ngay lập tức. | High |

---

## 4. Non-Functional Requirements

### 4.1 Technology Constraints
| ID | Requirement |
| :--- | :--- |
| NFR-TEC-01 | Hỗ trợ GUI. |
| NFR-TEC-02 | Ngôn ngữ triển khai: C/C++. |
| NFR-TEC-03 | Kiến trúc hệ thống tuân thủ MVC Pattern. |
| NFR-TEC-04 | Dependencies: SDL2 (Media), TagLib (Metadata), ImGui (GUI). |

### 4.2 Performance
| ID | Requirement |
| :--- | :--- |
| NFR-PER-01 | Audio Playback phải Non-blocking. |
| NFR-PER-02 | UI Response Time < 500ms. |

### 4.3 Reliability & Usability
| ID | Requirement |
| :--- | :--- |
| NFR-REL-01 | Exception Handling cho I/O tránh crash ứng dụng. |
| NFR-OPS-01 | Tự động phát hiện sự kiện USB Hot-plug. |
| NFR-REL-02 | Hệ thống không được crash khi mất kết nối Serial, tự động chuyển sang chế độ chỉ dùng GUI |

### 4.4 Resource Limits
| ID | Requirement |
| :--- | :--- |
| NFR-RES-01 | Maximum 10,000 files per Library. Khi đạt limit, hệ thống từ chối thêm file mới và hiển thị warning. |
| NFR-RES-02 | Maximum 1,000 tracks per Playlist. User không thể add thêm khi đạt limit. |
| NFR-RES-03 | History limited to 100 most recent tracks. Oldest entries auto-removed theo FIFO. |
| NFR-RES-04 | Maximum recursion depth = 10 levels khi scan directory. Deeper folders bị bỏ qua. |
| NFR-RES-05 | Maximum file path length = 4096 characters (Linux PATH_MAX). Longer paths rejected. |

---

## 5. Use Cases

### UC-00: System Startup & Restore Session 
| Field | Description |
| :--- | :--- |
| **Actor** | System |
| **Precondition** |Ứng dụng được khởi chạy. File cấu hình và dữ liệu (JSON) tồn tại trên đĩa. |
| **Main Flow** | 1. Hệ thống khởi tạo các Service (SDL2, UART, Logger). |
| | 2. Load Config: Hệ thống đọc file config.json để lấy cài đặt (Volume, Theme, Path). |
| | 3. Load Library: Hệ thống đọc file library.json, tái tạo cấu trúc dữ liệu Library |
| | 4. Load Playlists: Hệ thống quét thư mục lưu playlist, đọc các file *.json và hiển thị danh sách Playlist. |
| | 5. Restore State: Hệ thống xác định bài hát đang nghe dở (nếu có) và thiết lập trạng thái Pause tại vị trí đó. |
| | 6. Hệ thống hiển thị giao diện chính (Main GUI) với đầy đủ dữ liệu đã load. |
| **Alt Flow** | 3a. Corrupted Data: File dữ liệu bị lỗi/hỏng. Hệ thống hiển thị thông báo "Data corrupted, reset library" và tạo Library mới rỗng. |
| | 3b. First Run: Lần đầu chạy chưa có dữ liệu. Hệ thống tạo các file cấu hình mặc định |

### UC-01: Browse Media Files
| Field | Description |
| :--- | :--- |
| **Actor** | User |
| **Precondition** | Ứng dụng đang chạy. |
| **Main Flow** | 1. User chọn chức năng "Browse Files". |
| | 2. Hệ thống quét bộ nhớ (Local/USB) tìm file media hỗ trợ. |
| | 3. Hệ thống hiển thị danh sách file (phân trang). |
| | 4. User điều hướng danh sách. |
| | 5. User chọn file media. |
| | 6. User chọn "Add to Playlist/Library". |
| | 7. Nếu là Playlist: Hệ thống hiển thị danh sách playlist. |
| | 8. User chọn playlist hoặc "Create new playlist". |
| | 9. Hệ thống thêm file media vào playlist/library. |
| **Alt Flow** | 9a. File đã tồn tại trong playlist: Hiển thị thông báo duplicate yêu cầu xác nhận. |
| **Post-condition** | Giao diện hiển thị danh sách file media/thư mục. Người dùng có thể nhìn thấy và chọn các file. |

### UC-02: Create Playlist
| Field | Description |
| :--- | :--- |
| **Actor** | User |
| **Precondition** | Ứng dụng đang chạy. |
| **Main Flow** | 1. User chọn chức năng "Create Playlist". |
| | 2. User nhập tên playlist. |
| | 3. Hệ thống validate tên và khởi tạo playlist rỗng. |
| **Alt Flow** | 3a. Tên tồn tại: Yêu cầu nhập tên khác hoặc thêm số vào tên. |
| **Post-condition** | Một file playlist mới (.json) được tạo trên ổ cứng. Tên playlist mới xuất hiện trong danh sách Playlist trên giao diện. |

### UC-03: Play Song
| Field | Description |
| :--- | :--- |
| **Actor** | User |
| **Precondition** | File media được chọn. |
| **Main Flow** | 1. User gửi lệnh "Play". |
| | 2. Hệ thống cập nhật History và Playback Back Stack. |
| | 3. Hệ thống bắt đầu stream audio/video qua SDL2. |
| | 4. Hệ thống cập nhật UI (Progress, Metadata). |
| | 5. Play xong: Loop nếu enable, Stop nếu đang phát trong Playlist không còn bài nào khác, Next nếu còn bài |
| **Alt Flow** | 3a. File corrupted (bị xoá,...): Hiển thị thông báo lỗi và chuyển sang bài tiếp theo. |
| **Post-condition** | Âm thanh được phát ra loa. |
| | UI chuyển sang trạng thái "Playing" (Icon Pause hiển thị). |
| | Bài hát được thêm vào đầu danh sách History. |
| | Metadata bài hát được gửi xuống LCD (qua UART). |

### UC-04: Control via Hardware
| Field | Description |
| :--- | :--- |
| **Actor** | Hardware User (thông qua S32K144) |
| **Precondition** | Board kết nối thành công qua Serial. |
| **Main Flow** | 1. User nhấn nút vật lý (ví dụ: Next). |
| | 2. Board gửi command qua UART. |
| | 3. Hệ thống nhận và giải mã tín hiệu. |
| | 4. Hệ thống thực thi Action tương ứng (Next Track). |
| **Alt Flow** | 4a. Volume được điều chỉnh trước đó thông qua GUI: Volume jump theo ADC |
| **Post-condition** | Trạng thái phát nhạc (Play/Pause/Next/Previous) được cập nhật theo lệnh phần cứng. Volume được đồng bộ với giá trị ADC. |

### UC-05: Searching
| Field | Description |
| :--- | :--- |
| **Actor** | User |
| **Precondition** | Ứng dụng đang chạy. |
| | User đang ở màn hình Library hoặc Duyệt file |
| **Main Flow** | 1. User nhập từ khóa tìm kiếm. |
| | 2. Hệ thống tìm kiếm theo Metadata |
| | 3. Hệ thống hiển thị danh sách file media tìm được. |
| | 4. User thao tác với file media. |
| | 5. Hệ thống làm mới (Refresh) thông tin hiển thị. |
| **Post-condition** | Danh sách file media được hiển thị trên giao diện. Các file hợp lệ có thể được chọn để phát hoặc thêm vào playlist. |

### UC-06: Edit History
| Field | Description |
| :--- | :--- |
| **Actor** | User |
| **Precondition** | Ứng dụng đang chạy. |
| | User đang ở màn hình History. |
| **Main Flow** | 1. User trỏ vào file media và nhấn nút xoá. |
| | 2. Hệ thống xoá file media khỏi danh sách. |
| | 3. Hệ thống làm mới (Refresh) thông tin hiển thị. |
| | 4. Hệ thống làm mới (Refresh) thông tin hiển thị. |
| **Alt Flow** | 1a. User chọn "Clear All": Hệ thống xoá toàn bộ danh sách |
| **Post-condition** | Dữ liệu History trong RAM và trong file lưu trữ (history.json) được cập nhật (xóa bớt hoặc xóa hết). |

---
