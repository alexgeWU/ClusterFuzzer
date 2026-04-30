#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────
// Supported file types in the mock FS
// ─────────────────────────────────────────
enum class FileType {
    Text,
    Csv,
    Unknown
};

std::string fileTypeToString(FileType type);
FileType inferFileTypeFromExtension(const std::string& extension);
bool isSupportedFileExtension(const std::string& extension);

// ─────────────────────────────────────────
// Represents a single file in the FS
// ─────────────────────────────────────────
struct File {
    std::string name;       // full filename, e.g. notes.txt or logo.png
    std::string baseName;   // filename without extension, e.g. notes
    std::string extension;  // normalized extension without '.', e.g. txt/png
    FileType    type;
    std::string content;    // text or CSV content
    std::string password;   // empty = no protection
    bool        locked;     // true if password-protected

    File();
    File(const std::string& name,
         const std::string& content,
         const std::string& password = "");
};

// ─────────────────────────────────────────
// Represents a directory node
// ─────────────────────────────────────────
struct Directory {
    std::string name;
    std::unordered_map<std::string, File>      files;
    std::unordered_map<std::string, Directory> subdirs;

    Directory() = default;
    explicit Directory(const std::string& name);
};

// ─────────────────────────────────────────
// Mock Filesystem
// ─────────────────────────────────────────
class FileSystem {
public:
    FileSystem();

    // Navigation
    bool        changeDirectory(const std::string& path);
    std::string currentPath() const;
    void        listDirectory() const;

    // File operations
    bool createFile(const std::string& name,
                    const std::string& content,
                    const std::string& password = "");

    bool createTextFile(const std::string& name,
                        const std::string& content,
                        const std::string& password = "");

    bool createCsvFile(const std::string& name,
                       const std::string& content,
                       const std::string& password = "");

    bool displayFile(const std::string& name,
                     const std::string& password = "");

    bool deleteFile(const std::string& name,
                    const std::string& password = "");

    bool editFile(const std::string& name,
                  const std::string& newContent,
                  const std::string& password = "");

    bool setPassword(const std::string& name,
                     const std::string& oldPassword,
                     const std::string& newPassword);

    bool removePassword(const std::string& name,
                        const std::string& password);

    // Directory operations
    bool makeDirectory(const std::string& name);
    bool removeDirectory(const std::string& name);

    // Help
    void printHelp() const;

private:
    Directory                root_;
    std::vector<std::string> pathStack_;   // tracks current location

    // Helpers
    Directory*       currentDir();
    const Directory* currentDir() const;
    Directory*       navigateTo(const std::string& path);
    bool             authenticate(const File& file, const std::string& password) const;
    std::string      pathString() const;
};

// ─────────────────────────────────────────
// CLI shell loop
// ─────────────────────────────────────────
void runShell(FileSystem& fs, std::istream& in = std::cin);
