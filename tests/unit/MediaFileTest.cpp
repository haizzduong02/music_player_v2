#include "app/model/MediaFile.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <json.hpp>

namespace fs = std::filesystem;

class MediaFileTest : public ::testing::Test
{
  protected:
    std::string tempDir;
    std::string audioFile;
    std::string videoFile;
    std::string imageFile;
    std::string unknownFile;

    void SetUp() override
    {
        // Create a unique temporary directory for this test using timestamp to guarantee uniqueness
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        auto tempPath = fs::temp_directory_path() / ("MediaFileTest_" + std::to_string(timestamp));
        fs::create_directories(tempPath);
        tempDir = tempPath.string();

        audioFile = (tempPath / "test_audio.mp3").string();
        videoFile = (tempPath / "test_video.mp4").string();
        imageFile = (tempPath / "test_image.jpg").string();
        unknownFile = (tempPath / "test_doc.txt").string();

        createFile(audioFile, "audio content");
        createFile(videoFile, "video content");
        createFile(imageFile, "image content");
        // Unknown file not created yet for some tests
    }

    void TearDown() override
    {
        fs::remove_all(tempDir);
    }

    void createFile(const std::string &path, const std::string &content)
    {
        std::ofstream ofs(path);
        ofs << content;
        ofs.close();
    }
};

TEST_F(MediaFileTest, ConstructorParsesPathCorrectly)
{
    MediaFile file(audioFile);
    EXPECT_EQ(file.getPath(), audioFile);
    EXPECT_EQ(file.getFileName(), "test_audio.mp3");
    EXPECT_EQ(file.getExtension(), ".mp3");
    EXPECT_FALSE(file.isInLibrary());
}

TEST_F(MediaFileTest, ConstructorWithMetadata)
{
    MediaMetadata meta;
    meta.title = "Test Title";
    meta.artist = "Test Artist";

    MediaFile file(audioFile, meta);
    EXPECT_EQ(file.getPath(), audioFile);
    EXPECT_EQ(file.getMetadata().title, "Test Title");
    EXPECT_EQ(file.getMetadata().artist, "Test Artist");
}

TEST_F(MediaFileTest, DetermineMediaTypeExhaustive)
{
    // Audio
    EXPECT_EQ(MediaFile("t.mp3").getType(), MediaType::AUDIO);
    EXPECT_EQ(MediaFile("t.flac").getType(), MediaType::AUDIO);
    EXPECT_EQ(MediaFile("t.wav").getType(), MediaType::AUDIO);
    EXPECT_EQ(MediaFile("t.m4a").getType(), MediaType::AUDIO);
    EXPECT_EQ(MediaFile("t.aac").getType(), MediaType::AUDIO);
    EXPECT_EQ(MediaFile("t.ogg").getType(), MediaType::AUDIO);
    EXPECT_EQ(MediaFile("t.wma").getType(), MediaType::AUDIO);
    EXPECT_EQ(MediaFile("t.opus").getType(), MediaType::AUDIO);

    // Video
    EXPECT_EQ(MediaFile("t.mp4").getType(), MediaType::VIDEO);
    EXPECT_EQ(MediaFile("t.mkv").getType(), MediaType::VIDEO);
    EXPECT_EQ(MediaFile("t.avi").getType(), MediaType::VIDEO);
    EXPECT_EQ(MediaFile("t.mov").getType(), MediaType::VIDEO);
    EXPECT_EQ(MediaFile("t.wmv").getType(), MediaType::VIDEO);
    EXPECT_EQ(MediaFile("t.flv").getType(), MediaType::VIDEO);
    EXPECT_EQ(MediaFile("t.webm").getType(), MediaType::VIDEO);

    // Image
    EXPECT_EQ(MediaFile("t.jpg").getType(), MediaType::IMAGE);
    EXPECT_EQ(MediaFile("t.jpeg").getType(), MediaType::IMAGE);
    EXPECT_EQ(MediaFile("t.png").getType(), MediaType::IMAGE);
    EXPECT_EQ(MediaFile("t.gif").getType(), MediaType::IMAGE);
    EXPECT_EQ(MediaFile("t.bmp").getType(), MediaType::IMAGE);
    EXPECT_EQ(MediaFile("t.webp").getType(), MediaType::IMAGE);

    // Unknown
    EXPECT_EQ(MediaFile("t.txt").getType(), MediaType::UNKNOWN);
    EXPECT_EQ(MediaFile("t").getType(), MediaType::UNKNOWN);
}

TEST_F(MediaFileTest, GetDisplayNameWithTitle)
{
    MediaMetadata meta;
    meta.title = "My Song";
    MediaFile file(audioFile, meta);
    EXPECT_EQ(file.getDisplayName(), "My Song");
}

TEST_F(MediaFileTest, GetDisplayNameWithoutTitle)
{
    MediaFile file(audioFile);
    EXPECT_EQ(file.getDisplayName(), "test_audio");
}

TEST_F(MediaFileTest, GetDisplayNameRemovesPrefixes)
{
    std::string nastyFile = (fs::path(tempDir) / "y2mate.com - Cool Song.mp3").string();
    MediaFile file1(nastyFile);
    EXPECT_EQ(file1.getDisplayName(), "Cool Song");

    std::string nastyFile2 = (fs::path(tempDir) / "y2mate.is - Another Song.mp3").string();
    MediaFile file2(nastyFile2);
    EXPECT_EQ(file2.getDisplayName(), "Another Song");

    std::string normalFile = (fs::path(tempDir) / "Normal Song.mp3").string();
    MediaFile file3(normalFile);
    EXPECT_EQ(file3.getDisplayName(), "Normal Song");
}

TEST_F(MediaFileTest, Exists)
{
    MediaFile file(audioFile);
    EXPECT_TRUE(file.exists());

    MediaFile missing((fs::path(tempDir) / "missing.mp3").string());
    EXPECT_FALSE(missing.exists());
}

TEST_F(MediaFileTest, GetFileSize)
{
    MediaFile file(audioFile);
    // "audio content" is 13 bytes
    EXPECT_EQ(file.getFileSize(), 13);

    MediaFile missing((fs::path(tempDir) / "missing.mp3").string());
    EXPECT_EQ(missing.getFileSize(), 0);
}

TEST_F(MediaFileTest, Setters)
{
    MediaFile file(audioFile);
    EXPECT_FALSE(file.isInLibrary());

    file.setInLibrary(true);
    EXPECT_TRUE(file.isInLibrary());

    MediaMetadata meta;
    meta.album = "New Album";
    file.setMetadata(meta);
    EXPECT_EQ(file.getMetadata().album, "New Album");
}

TEST_F(MediaFileTest, JsonSerializationComprehensive)
{
    MediaMetadata meta;
    meta.title = "JSON Title";
    meta.artist = "JSON Artist";
    meta.album = "JSON Album";
    meta.genre = "JSON Genre";
    meta.year = 2024;
    meta.track = 5;
    meta.duration = 120;

    MediaFile original(audioFile, meta);
    original.setInLibrary(true);

    nlohmann::json j;
    to_json(j, original);

    MediaFile restored("");
    from_json(j, restored);

    EXPECT_EQ(restored.getPath(), audioFile);
    EXPECT_EQ(restored.getMetadata().title, "JSON Title");
    EXPECT_EQ(restored.getMetadata().artist, "JSON Artist");
    EXPECT_EQ(restored.getMetadata().album, "JSON Album");
    EXPECT_EQ(restored.getMetadata().genre, "JSON Genre");
    EXPECT_EQ(restored.getMetadata().year, 2024);
    EXPECT_EQ(restored.getMetadata().track, 5);
    EXPECT_EQ(restored.getMetadata().duration, 120);
    EXPECT_TRUE(restored.isInLibrary());
}

TEST_F(MediaFileTest, JsonDeserializationPartial)
{
    // Minimal JSON
    nlohmann::json minJ = {{"path", "/test/min.mp3"}};
    MediaFile m1("");
    from_json(minJ, m1);
    EXPECT_EQ(m1.getPath(), "/test/min.mp3");
    EXPECT_EQ(m1.getType(), MediaType::AUDIO);

    // JSON without inLibrary
    nlohmann::json noLibJ = {{"path", "/test/noLib.mp3"}, {"metadata", {{"title", "T"}}}};
    MediaFile m2("");
    from_json(noLibJ, m2);
    EXPECT_EQ(m2.getMetadata().title, "T");
    EXPECT_FALSE(m2.isInLibrary());

    // JSON with individual metadata missing
    nlohmann::json partialMetaJ = {
        {"path", "/test.mp3"},
        {"metadata", {{"artist", "A"}, {"year", 1999}}}
    };
    MediaFile m3("");
    from_json(partialMetaJ, m3);
    EXPECT_EQ(m3.getMetadata().artist, "A");
    EXPECT_EQ(m3.getMetadata().year, 1999);
    EXPECT_EQ(m3.getMetadata().title, ""); // Default
}
