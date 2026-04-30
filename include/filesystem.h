#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

// ─────────────────────────────────────────
// Represents a single text file in the FS
// ─────────────────────────────────────────
struct File {
    std::string name;
    std::string content;
    std::string password;   // empty = no protection
    bool        locked;     // true if password-protected

    File() : locked(false) {}
    File(const std::string& name, const std::string& content, const std::string& password = "");
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
void runShell(FileSystem& fs);