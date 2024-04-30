#include <vector>
#include <string>
#include <map>

class FileTypeDetector // map file extensions to file types (e.g. "html" -> "text/html")
{
private:
    std::map<std::vector<std::string>, std::string> _fileTypes;

public:
    FileTypeDetector();
    FileTypeDetector(const std::map<std::vector<std::string>, std::string> &fileTypes);
    void addFileType(const std::vector<std::string> &extensions, const std::string &type);
    void addSingleFileType(const std::string &extension, const std::string &type);
    std::string getFileType(const std::string &extension);
};
