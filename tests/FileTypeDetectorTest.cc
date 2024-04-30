#include <gtest/gtest.h>
#include "../FileTypeDetector.hpp"

class FileTypeDetectorTest : public ::testing::Test // TEXT FIXTURE !
{
protected:
    FileTypeDetectorTest() : instance(){};
    void SetUp() override
    {
        instance.addSingleFileType("html", "text/html");
        instance.addSingleFileType("css", "text/css");
        instance.addSingleFileType("js", "text/javascript");
        instance.addSingleFileType("txt", "");
    };
    void TearDown() override{};
    FileTypeDetector instance;
};

TEST_F(FileTypeDetectorTest, GetFileType)
{
    EXPECT_EQ(instance.getFileType("html"), "text/html");
    EXPECT_EQ(instance.getFileType("css"), "text/css");
    EXPECT_EQ(instance.getFileType("js"), "text/javascript");
    EXPECT_EQ(instance.getFileType("txt"), "");
}

TEST_F(FileTypeDetectorTest, GetFileTypeEmpty)
{
    EXPECT_EQ(instance.getFileType("unknown"), "");
}