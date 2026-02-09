#include "gtest/gtest.h"
#include "interfaces/IMetadataReader.h"

TEST(MetadataDefaultsTest, MembersAreInitialized) {
    MediaMetadata meta;
    
    // Integers should be 0
    EXPECT_EQ(meta.duration, 0);
    EXPECT_EQ(meta.bitrate, 0);
    EXPECT_EQ(meta.sampleRate, 0);
    EXPECT_EQ(meta.channels, 0);
    EXPECT_EQ(meta.year, 0);
    EXPECT_EQ(meta.track, 0);
    
    // Bool should be false
    EXPECT_FALSE(meta.hasAlbumArt);
    
    // Strings should be empty (default std::string behavior)
    EXPECT_TRUE(meta.title.empty());
    EXPECT_TRUE(meta.artist.empty());
}
