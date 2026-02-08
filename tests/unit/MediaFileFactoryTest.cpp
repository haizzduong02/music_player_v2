#include "app/model/MediaFileFactory.h"
#include "tests/mocks/MockMetadataReader.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;

class MediaFileFactoryTest : public ::testing::Test
{
  protected:
    MockMetadataReader mockReader;
};

// createMediaFile tests
TEST_F(MediaFileFactoryTest, CreateMediaFileWithoutReader)
{
    auto file = MediaFileFactory::createMediaFile("/test/song.mp3", nullptr);
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(file->getPath(), "/test/song.mp3");
}

TEST_F(MediaFileFactoryTest, CreateMediaFileWithReader)
{
    MediaMetadata metadata;
    metadata.title = "Test Song";
    metadata.artist = "Test Artist";
    metadata.album = "Test Album";

    EXPECT_CALL(mockReader, readMetadata("/test/song.mp3"))
        .WillOnce(Return(metadata));

    auto file = MediaFileFactory::createMediaFile("/test/song.mp3", &mockReader);
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(file->getMetadata().title, "Test Song");
    EXPECT_EQ(file->getMetadata().artist, "Test Artist");
}

TEST_F(MediaFileFactoryTest, CreateMediaFileWithReaderException)
{
    EXPECT_CALL(mockReader, readMetadata("/test/song.mp3"))
        .WillOnce(Throw(std::runtime_error("Failed to read metadata")));

    auto file = MediaFileFactory::createMediaFile("/test/song.mp3", &mockReader);
    ASSERT_NE(file, nullptr);
    // Should still create file, just without metadata
    EXPECT_EQ(file->getPath(), "/test/song.mp3");
}

// createMediaFileWithMetadata tests
TEST_F(MediaFileFactoryTest, CreateMediaFileWithMetadata)
{
    MediaMetadata metadata;
    metadata.title = "Preset Song";
    metadata.artist = "Preset Artist";

    auto file = MediaFileFactory::createMediaFileWithMetadata("/test/song.mp3", metadata);
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(file->getMetadata().title, "Preset Song");
    EXPECT_EQ(file->getMetadata().artist, "Preset Artist");
}

// isSupportedFormat tests
TEST_F(MediaFileFactoryTest, IsSupportedFormatWithDot)
{
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat(".mp3"));
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat(".flac"));
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat(".mp4"));
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat(".mkv"));
}

TEST_F(MediaFileFactoryTest, IsSupportedFormatWithoutDot)
{
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat("mp3"));
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat("flac"));
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat("mp4"));
}

TEST_F(MediaFileFactoryTest, IsSupportedFormatCaseInsensitive)
{
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat(".MP3"));
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat(".FLAC"));
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat("MP4"));
    EXPECT_TRUE(MediaFileFactory::isSupportedFormat(".MkV"));
}

TEST_F(MediaFileFactoryTest, IsSupportedFormatUnsupported)
{
    EXPECT_FALSE(MediaFileFactory::isSupportedFormat(".txt"));
    EXPECT_FALSE(MediaFileFactory::isSupportedFormat(".pdf"));
    EXPECT_FALSE(MediaFileFactory::isSupportedFormat("doc"));
    EXPECT_FALSE(MediaFileFactory::isSupportedFormat(""));
}

// Format listing tests
TEST_F(MediaFileFactoryTest, GetSupportedAudioFormats)
{
    auto formats = MediaFileFactory::getSupportedAudioFormats();
    EXPECT_FALSE(formats.empty());
    EXPECT_NE(std::find(formats.begin(), formats.end(), ".mp3"), formats.end());
    EXPECT_NE(std::find(formats.begin(), formats.end(), ".flac"), formats.end());
    EXPECT_NE(std::find(formats.begin(), formats.end(), ".wav"), formats.end());
}

TEST_F(MediaFileFactoryTest, GetSupportedVideoFormats)
{
    auto formats = MediaFileFactory::getSupportedVideoFormats();
    EXPECT_FALSE(formats.empty());
    EXPECT_NE(std::find(formats.begin(), formats.end(), ".mp4"), formats.end());
    EXPECT_NE(std::find(formats.begin(), formats.end(), ".mkv"), formats.end());
}

TEST_F(MediaFileFactoryTest, GetAllSupportedFormats)
{
    auto all = MediaFileFactory::getAllSupportedFormats();
    auto audio = MediaFileFactory::getSupportedAudioFormats();
    auto video = MediaFileFactory::getSupportedVideoFormats();

    EXPECT_EQ(all.size(), audio.size() + video.size());

    // Check all audio formats are included
    for (const auto& fmt : audio)
    {
        EXPECT_NE(std::find(all.begin(), all.end(), fmt), all.end());
    }

    // Check all video formats are included
    for (const auto& fmt : video)
    {
        EXPECT_NE(std::find(all.begin(), all.end(), fmt), all.end());
    }
}
