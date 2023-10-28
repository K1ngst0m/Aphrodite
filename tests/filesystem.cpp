#include <catch2/catch_test_macros.hpp>
#include "filesystem/filesystem.h"
#include <fstream>
#include <utility>

class FileGuard
{
public:
    explicit FileGuard(std::string filename) : m_filename(std::move(filename)) {}

    ~FileGuard() { std::remove(m_filename.c_str()); }

private:
    std::string m_filename;
};

using namespace aph;

TEST_CASE("Test Filesystem Protocol Setup", "[Filesystem]")
{
    Filesystem fs;

    SECTION("Default protocols are set up")
    {
        REQUIRE(fs.protocolExists("assets"));
        REQUIRE(fs.protocolExists("models"));
        REQUIRE(fs.protocolExists("fonts"));
        REQUIRE(fs.protocolExists("shader_glsl"));
        REQUIRE(fs.protocolExists("shader_slang"));
        REQUIRE(fs.protocolExists("textures"));
    }

    SECTION("Add new protocol")
    {
        fs.registerProtocol("newprotocol", "/some/path");
        REQUIRE(fs.protocolExists("newprotocol"));
    }

    SECTION("Remove existing protocol")
    {
        fs.removeProtocol("assets");
        REQUIRE_FALSE(fs.protocolExists("assets"));
    }
}

// Utility to create a temporary test file.
std::string createTempFile(const std::string& content)
{
    static int  tempFileCount = 0;
    std::string tempFileName  = "tempFile_" + std::to_string(tempFileCount++) + ".txt";

    std::ofstream out(tempFileName);
    out << content;
    out.close();

    return tempFileName;
}

TEST_CASE("Filesystem Basic Operations", "[Filesystem]")
{
    Filesystem fs;

    std::string testContent  = "Hello, World!";
    std::string tempFilePath = createTempFile(testContent);

    SECTION("Mapping and Unmapping Files")
    {
        auto* mapped = fs.map(tempFilePath);
        REQUIRE(mapped != nullptr);  // Ensure mapping returns a non-null pointer.
        fs.unmap(mapped);
    }

    SECTION("Reading File to String")
    {
        std::string content = fs.readFileToString(tempFilePath);
        REQUIRE(content == testContent);  // Check if the content read is correct.
    }

    SECTION("Reading File to Bytes")
    {
        std::vector<uint8_t> bytes = fs.readFileToBytes(tempFilePath);
        REQUIRE(bytes.size() ==
                testContent.size());  // Simple size check. You can add more specific byte-by-byte checks.
    }

    SECTION("Reading File Lines")
    {
        std::string multiLineContent  = "Line1\nLine2\nLine3";
        std::string multiLineFilePath = createTempFile(multiLineContent);

        std::vector<std::string> lines = fs.readFileLines(multiLineFilePath);
        REQUIRE(lines.size() == 3);  // We added 3 lines.
        REQUIRE(lines[0] == "Line1");
        REQUIRE(lines[1] == "Line2");
        REQUIRE(lines[2] == "Line3");
    }

    // Clean up the temporary files after tests.
    std::remove(tempFilePath.c_str());
}

// Utility to read a file into a string for verification purposes.
std::string readFileToString(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    std::string   content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return content;
}
TEST_CASE("Filesystem Write Operations", "[Filesystem]")
{
    Filesystem fs;

    std::string testFile = "writeTestFile.txt";

    SECTION("Writing String to File")
    {
        std::string content = "Hello, Write!";
        fs.writeStringToFile(testFile, content);

        std::string readBack = readFileToString(testFile);
        REQUIRE(readBack == content);

        std::remove(testFile.c_str());  // Clean up after the test.
    }

    SECTION("Writing Bytes to File")
    {
        std::vector<uint8_t> bytes = {72, 101, 108, 108, 111};  // ASCII for "Hello"
        fs.writeBytesToFile(testFile, bytes);

        std::string readBack = readFileToString(testFile);
        REQUIRE(readBack == "Hello");

        std::remove(testFile.c_str());  // Clean up after the test.
    }

    SECTION("Writing Lines to File")
    {
        std::vector<std::string> lines = {"Line1", "Line2", "Line3"};
        fs.writeLinesToFile(testFile, lines);

        std::string readBack = readFileToString(testFile);
        REQUIRE(readBack == "Line1\nLine2\nLine3\n");  // Note: The method adds '\n' after each line.

        std::remove(testFile.c_str());  // Clean up after the test.
    }
}
