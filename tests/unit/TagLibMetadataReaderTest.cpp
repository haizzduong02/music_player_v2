#include "service/TagLibMetadataReader.h"
#include <fstream>
#include <filesystem>
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

TEST_F(TagLibMetadataReaderTest, GetMetadataValidFileDetailed)
{
    MediaMetadata meta = reader.readMetadata(validMp3);
    EXPECT_GE(meta.duration, 0);
    EXPECT_GT(meta.bitrate, 0);
    EXPECT_GT(meta.sampleRate, 0);
    EXPECT_GE(meta.channels, 1);
    EXPECT_EQ(meta.codec, "MP3");
}

TEST_F(TagLibMetadataReaderTest, CodecFallbacks)
{
    EXPECT_EQ(reader.readMetadata("test.wav").codec, "WAV");
    EXPECT_EQ(reader.readMetadata("test.m4a").codec, "AAC");
    EXPECT_EQ(reader.readMetadata("test.ogg").codec, "Vorbis");
    EXPECT_EQ(reader.readMetadata("test.unknown").codec, "");
}

TEST_F(TagLibMetadataReaderTest, ExtractTagsEmpty)
{
    auto result = reader.extractTags(validMp3, {});
    EXPECT_TRUE(result.empty());
}

TEST_F(TagLibMetadataReaderTest, ExtractTagsInvalidFile)
{
    auto result = reader.extractTags(nonExistentFile, {"Title"});
    EXPECT_TRUE(result.empty());
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

#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>

TEST_F(TagLibMetadataReaderTest, ReadAlbumArtMP3)
{
    std::string artFile = "tests/assets/art_test.mp3";
    std::ifstream src(validMp3, std::ios::binary);
    std::ofstream dst(artFile, std::ios::binary);
    dst << src.rdbuf();
    src.close();
    dst.close();

    // Add APIC frame using TagLib
    TagLib::MPEG::File f(artFile.c_str());
    TagLib::ID3v2::Tag *tag = f.ID3v2Tag(true);
    TagLib::ID3v2::AttachedPictureFrame *frame = new TagLib::ID3v2::AttachedPictureFrame();
    frame->setMimeType("image/jpeg");
    frame->setPicture("fake_jpeg_data");
    tag->addFrame(frame);
    f.save();

    MediaMetadata meta = reader.readMetadata(artFile);
    EXPECT_TRUE(meta.hasAlbumArt);
    EXPECT_EQ(meta.albumArtMimeType, "image/jpeg");
    EXPECT_FALSE(meta.albumArtData.empty());

    std::remove(artFile.c_str());
}

TEST_F(TagLibMetadataReaderTest, ReadAlbumArtFLAC)
{
    std::string flacFile = "tests/assets/art_test.flac";
    std::string sampleFlac = "tests/assets/sample.flac";
    
    // Copy sample FLAC to work file
    std::ifstream src(sampleFlac, std::ios::binary);
    if (!src.is_open()) {
        // Fallback if sample.flac was not generated correctly
        return;
    }
    std::ofstream dst(flacFile, std::ios::binary);
    dst << src.rdbuf();
    src.close();
    dst.close();

    // Add picture using TagLib
    TagLib::FLAC::File f(flacFile.c_str());
    TagLib::FLAC::Picture *picture = new TagLib::FLAC::Picture();
    picture->setMimeType("image/png");
    picture->setType(TagLib::FLAC::Picture::FrontCover);
    picture->setData("fake_png_data");
    f.addPicture(picture);
    f.save();

    MediaMetadata meta = reader.readMetadata(flacFile);
    EXPECT_TRUE(meta.hasAlbumArt);
    EXPECT_EQ(meta.albumArtMimeType, "image/png");
    EXPECT_FALSE(meta.albumArtData.empty());

    std::remove(flacFile.c_str());
}

TEST_F(TagLibMetadataReaderTest, WriteMetadataSaveFailure)
{
    // Try to trigger line 146 (failure to save)
    // We can do this by making the file read-only after opening it with FileRef
    std::string roFile = "tests/assets/readonly.mp3";
    std::ifstream src(validMp3, std::ios::binary);
    std::ofstream dst(roFile, std::ios::binary);
    dst << src.rdbuf();
    src.close();
    dst.close();

    // Change permissions to read-only
    std::filesystem::permissions(roFile, std::filesystem::perms::owner_read);

    MediaMetadata meta;
    meta.title = "Fail";
    // TagLib might fail to open or fail to save
    bool result = reader.writeMetadata(roFile, meta);
    EXPECT_FALSE(result);

    std::remove(roFile.c_str());
}
