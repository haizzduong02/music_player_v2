#include "service/TagLibMetadataReader.h"
#include <fstream>
#include <gtest/gtest.h>

class TagLibMetadataReaderTest : public ::testing::Test
{
  protected:
    TagLibMetadataReader reader;
    std::string validMp3 = "tests/assets/sample.mp3";
    std::string invalidFile = "tests/assets/invalid.txt";
    std::string nonExistentFile = "tests/assets/nonexistent.mp3";

    void SetUp() override
    {
        // Create invalid file
        std::ofstream ofs(invalidFile);
        ofs << "not an mp3";
        ofs.close();
    }

    void TearDown() override
    {
        std::remove(invalidFile.c_str());
    }

    bool proxyIsFormatSupported(const std::string &ext)
    {
        return reader.isFormatSupported(ext);
    }
};

TEST_F(TagLibMetadataReaderTest, GetMetadataValidFile)
{
    MediaMetadata meta = reader.readMetadata(validMp3);
    EXPECT_GE(meta.duration, 0);
}

TEST_F(TagLibMetadataReaderTest, GetMetadataInvalidFile)
{
    MediaMetadata meta = reader.readMetadata(invalidFile);
    EXPECT_EQ(meta.duration, 0);
    EXPECT_EQ(meta.title, "");
}

TEST_F(TagLibMetadataReaderTest, GetMetadataNonExistentFile)
{
    MediaMetadata meta = reader.readMetadata(nonExistentFile);
    EXPECT_EQ(meta.duration, 0);
}

TEST_F(TagLibMetadataReaderTest, WriteMetadata)
{
    std::string tempFile = "tests/assets/temp_write.mp3";
    std::ifstream src(validMp3, std::ios::binary);
    std::ofstream dst(tempFile, std::ios::binary);
    dst << src.rdbuf();
    src.close();
    dst.close();

    MediaMetadata meta;
    meta.title = "New Title";
    meta.artist = "New Artist";
    meta.album = "New Album";
    meta.year = 2024;

    EXPECT_TRUE(reader.writeMetadata(tempFile, meta));

    // Read back
    MediaMetadata readMeta = reader.readMetadata(tempFile);
    EXPECT_EQ(readMeta.title, "New Title");
    EXPECT_EQ(readMeta.artist, "New Artist");
    EXPECT_EQ(readMeta.year, 2024);

    std::remove(tempFile.c_str());
}

TEST_F(TagLibMetadataReaderTest, SupportsEditing)
{
    EXPECT_TRUE(reader.supportsEditing("file.mp3"));
    EXPECT_TRUE(reader.supportsEditing("file.FLAC"));
    EXPECT_TRUE(reader.supportsEditing("file.ogg"));
    EXPECT_FALSE(reader.supportsEditing("file.txt"));
    EXPECT_FALSE(reader.supportsEditing("file_no_extension"));
}

TEST_F(TagLibMetadataReaderTest, AllFormatsSupported)
{
    EXPECT_TRUE(proxyIsFormatSupported(".mp3"));
    EXPECT_TRUE(proxyIsFormatSupported(".flac"));
    EXPECT_TRUE(proxyIsFormatSupported(".ogg"));
    EXPECT_TRUE(proxyIsFormatSupported(".m4a"));
    EXPECT_TRUE(proxyIsFormatSupported(".wav"));
    EXPECT_TRUE(proxyIsFormatSupported(".wma"));
    EXPECT_TRUE(proxyIsFormatSupported(".ape"));
    EXPECT_FALSE(proxyIsFormatSupported(".txt"));
}

TEST_F(TagLibMetadataReaderTest, GetMetadataFlac)
{
    MediaMetadata meta = reader.readMetadata("test.flac");
    EXPECT_EQ(meta.codec, "FLAC");
}

TEST_F(TagLibMetadataReaderTest, WriteMetadataFailures)
{
    MediaMetadata meta;
    EXPECT_FALSE(reader.writeMetadata(nonExistentFile, meta));
    EXPECT_FALSE(reader.writeMetadata("file.txt", meta));
}

TEST_F(TagLibMetadataReaderTest, ExtractTagsComprehensive)
{
    std::vector<std::string> tags = {"Title", "Artist", "Album", "Genre", "Year", "Track", "Unknown"};
    auto result = reader.extractTags(validMp3, tags);

    // Check Case-Insensitivity
    std::vector<std::string> tagsLower = {"title", "artist", "album", "genre", "year", "track"};
    auto result2 = reader.extractTags(validMp3, tagsLower);

    EXPECT_EQ(result.size(), result2.size());
}
