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

TEST_F(MediaFileTest, DetermineMediaType)
{
    MediaFile audio(audioFile);
    EXPECT_EQ(audio.getType(), MediaType::AUDIO);

    MediaFile video(videoFile);
    EXPECT_EQ(video.getType(), MediaType::VIDEO);

    MediaFile image(imageFile);
    EXPECT_EQ(image.getType(), MediaType::IMAGE);

    MediaFile unknown(unknownFile);
    EXPECT_EQ(unknown.getType(), MediaType::UNKNOWN);
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

TEST_F(MediaFileTest, JsonSerialization)
{
    MediaMetadata meta;
    meta.title = "JSON Title";
    meta.artist = "JSON Artist";
    meta.duration = 120;

    MediaFile original(audioFile, meta);
    original.setInLibrary(true);

    nlohmann::json j;
    to_json(j, original);

    MediaFile restored("");
    from_json(j, restored);

    EXPECT_EQ(restored.getPath(), audioFile);            // Path matches
    EXPECT_EQ(restored.getFileName(), "test_audio.mp3"); // Reparsed
    EXPECT_EQ(restored.getExtension(), ".mp3");          // Reparsed
    EXPECT_EQ(restored.getType(), MediaType::AUDIO);     // redetermined
    EXPECT_EQ(restored.getMetadata().title, "JSON Title");
    EXPECT_EQ(restored.getMetadata().artist, "JSON Artist");
    EXPECT_EQ(restored.getMetadata().duration, 120);
    EXPECT_TRUE(restored.isInLibrary());
}
