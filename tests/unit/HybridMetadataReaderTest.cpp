
#include "service/HybridMetadataReader.h"
#include "tests/mocks/MockMetadataReader.h"
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class HybridMetadataReaderTest : public ::testing::Test
{
  protected:
    std::unique_ptr<HybridMetadataReader> reader;
    MockMetadataReader *primaryMock;
    MockMetadataReader *secondaryMock;

    void SetUp() override
    {
        auto primary = std::make_unique<NiceMock<MockMetadataReader>>();
        primaryMock = primary.get();
        auto secondary = std::make_unique<NiceMock<MockMetadataReader>>();
        secondaryMock = secondary.get();

        reader = std::make_unique<HybridMetadataReader>(std::move(primary), std::move(secondary));
    }
};

TEST_F(HybridMetadataReaderTest, ReadMetadata_PrimarySuccess)
{
    MediaMetadata meta;
    meta.duration = 100;
    meta.title = "Primary Title";

    EXPECT_CALL(*primaryMock, readMetadata("test.mp3")).WillOnce(Return(meta));
    EXPECT_CALL(*secondaryMock, readMetadata(_)).Times(0);

    MediaMetadata result = reader->readMetadata("test.mp3");
    EXPECT_EQ(result.duration, 100);
    EXPECT_EQ(result.title, "Primary Title");
}

TEST_F(HybridMetadataReaderTest, ReadMetadata_PrimaryFailure)
{
    MediaMetadata primaryMeta;
    primaryMeta.duration = 0; // Trigger fallback
    
    MediaMetadata secondaryMeta;
    secondaryMeta.duration = 200;
    secondaryMeta.title = "Secondary Title";

    EXPECT_CALL(*primaryMock, readMetadata("test.mp4")).WillOnce(Return(primaryMeta));
    EXPECT_CALL(*secondaryMock, readMetadata("test.mp4")).WillOnce(Return(secondaryMeta));

    MediaMetadata result = reader->readMetadata("test.mp4");
    EXPECT_EQ(result.duration, 200);
    EXPECT_EQ(result.title, "Secondary Title");
}

TEST_F(HybridMetadataReaderTest, ReadMetadata_BothFail)
{
    MediaMetadata primaryMeta; 
    primaryMeta.duration = 0;
    
    MediaMetadata secondaryMeta;
    secondaryMeta.duration = 0;

    EXPECT_CALL(*primaryMock, readMetadata("test.unknown")).WillOnce(Return(primaryMeta));
    EXPECT_CALL(*secondaryMock, readMetadata("test.unknown")).WillOnce(Return(secondaryMeta));

    MediaMetadata result = reader->readMetadata("test.unknown");
    EXPECT_EQ(result.duration, 0);
}

TEST_F(HybridMetadataReaderTest, ReadMetadata_MergeResults)
{
    MediaMetadata primaryMeta;
    primaryMeta.duration = 0;
    primaryMeta.artist = "Primary Artist"; // Should stay
    
    MediaMetadata secondaryMeta;
    secondaryMeta.duration = 300;
    secondaryMeta.title = "Secondary Title"; // Should be added
    secondaryMeta.artist = "Secondary Artist"; // Should be ignored (primary has it)

    EXPECT_CALL(*primaryMock, readMetadata("test.mkv")).WillOnce(Return(primaryMeta));
    EXPECT_CALL(*secondaryMock, readMetadata("test.mkv")).WillOnce(Return(secondaryMeta));

    MediaMetadata result = reader->readMetadata("test.mkv");
    EXPECT_EQ(result.duration, 300);
    EXPECT_EQ(result.artist, "Primary Artist"); 
    EXPECT_EQ(result.title, "Secondary Title");
}

TEST_F(HybridMetadataReaderTest, WriteMetadata_UsePrimary)
{
    MediaMetadata meta;
    EXPECT_CALL(*primaryMock, supportsEditing("test.mp3")).WillOnce(Return(true));
    EXPECT_CALL(*primaryMock, writeMetadata("test.mp3", _)).WillOnce(Return(true));

    EXPECT_TRUE(reader->writeMetadata("test.mp3", meta));
}

TEST_F(HybridMetadataReaderTest, WriteMetadata_FailIfPrimaryUnsupported)
{
    MediaMetadata meta;
    EXPECT_CALL(*primaryMock, supportsEditing("test.mkv")).WillOnce(Return(false));
    // Secondary is typically not checked for writing in current implementation
    
    EXPECT_FALSE(reader->writeMetadata("test.mkv", meta));
}

TEST_F(HybridMetadataReaderTest, ExtractTags_PrimaryFirst)
{
    std::map<std::string, std::string> tags = {{"Title", "Foo"}};
    EXPECT_CALL(*primaryMock, extractTags("test.mp3", _)).WillOnce(Return(tags));
    
    auto result = reader->extractTags("test.mp3", {});
    EXPECT_EQ(result["Title"], "Foo");
}

TEST_F(HybridMetadataReaderTest, ExtractTags_Fallback)
{
    std::map<std::string, std::string> emptyTags;
    std::map<std::string, std::string> secondaryTags = {{"Title", "Bar"}};
    
    EXPECT_CALL(*primaryMock, extractTags("test.mkv", _)).WillOnce(Return(emptyTags));
    EXPECT_CALL(*secondaryMock, extractTags("test.mkv", _)).WillOnce(Return(secondaryTags));
    
    auto result = reader->extractTags("test.mkv", {});
    EXPECT_EQ(result["Title"], "Bar");
}

TEST_F(HybridMetadataReaderTest, SupportsEditing)
{
    EXPECT_CALL(*primaryMock, supportsEditing("a.mp3")).WillOnce(Return(true));
    EXPECT_TRUE(reader->supportsEditing("a.mp3"));

    EXPECT_CALL(*primaryMock, supportsEditing("b.mkv")).WillOnce(Return(false));
    EXPECT_CALL(*secondaryMock, supportsEditing("b.mkv")).WillOnce(Return(true));
    EXPECT_TRUE(reader->supportsEditing("b.mkv"));
    
    EXPECT_CALL(*primaryMock, supportsEditing("c.txt")).WillOnce(Return(false));
    EXPECT_CALL(*secondaryMock, supportsEditing("c.txt")).WillOnce(Return(false));
    EXPECT_FALSE(reader->supportsEditing("c.txt"));
}
