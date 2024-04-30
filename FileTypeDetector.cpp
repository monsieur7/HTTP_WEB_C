#include "FileTypeDetector.hpp"

FileTypeDetector::FileTypeDetector()
{
}

FileTypeDetector::FileTypeDetector(const std::map<std::vector<std::string>, std::string> &fileTypes)
{
    _fileTypes = fileTypes;
}

void FileTypeDetector::addFileType(const std::vector<std::string> &extensions, const std::string &type)
{
    _fileTypes[extensions] = type;
}

void FileTypeDetector::addSingleFileType(const std::string &extension, const std::string &type)
{
    std::vector<std::string> extensions;
    extensions.push_back(extension);
    addFileType(extensions, type);
}

std::string FileTypeDetector::getFileType(const std::string &extension)
{
    std::map<std::vector<std::string>, std::string>::iterator it;
    for (it = _fileTypes.begin(); it != _fileTypes.end(); it++)
    {
        std::vector<std::string> extensions = it->first;
        for (unsigned int i = 0; i < extensions.size(); i++)
        {
            if (extensions[i] == extension)
            {
                return it->second;
            }
        }
    }
    return "";
}
