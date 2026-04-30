#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────
// Permission bits
//
//  File context:
//    PERM_READ   (r) – read file content        (cat)
//    PERM_WRITE  (w) – edit file content        (edit / passwd)
//    PERM_DELETE (d) – delete the file          (rm)
//
//  Directory context:
//    PERM_READ   (r) – list directory entries   (ls)
//    PERM_WRITE  (w) – create or remove entries (touch / mkdir / rmdir / rm)
//    PERM_DELETE (x) – traverse / cd into       (cd)
// ─────────────────────────────────────────
constexpr uint8_t PERM_READ   = 0x4;   // 100
constexpr uint8_t PERM_WRITE  = 0x2;   // 010
constexpr uint8_t PERM_DELETE = 0x1;   // 001  (also used as traverse for dirs)
constexpr uint8_t PERM_ALL    = 0x7;   // 111
constexpr uint8_t PERM_NONE   = 0x0;   // 000

// ─────────────────────────────────────────
// Three-tier Unix-style permission set
// ─────────────────────────────────────────
struct Permission {
    uint8_t owner = PERM_ALL;                    // owner bits  (default: rwx / rwd)
    uint8_t group = PERM_READ | PERM_WRITE;      // group bits  (default: rw-)
    uint8_t other = PERM_READ;                   // other bits  (default: r--)

    // Construct from three octal digits (each clamped to [0,7])
    static Permission fromOctal(uint8_t o, uint8_t g, uint8_t oth) noexcept {
        return { static_cast<uint8_t>(o & 0x7),
                 static_cast<uint8_t>(g & 0x7),
                 static_cast<uint8_t>(oth & 0x7) };
    }

    bool valid()                           const noexcept { return owner <= 7 && group <= 7 && other <= 7; }
    bool operator==(const Permission& rhs) const noexcept { return owner == rhs.owner && group == rhs.group && other == rhs.other; }
    bool operator!=(const Permission& rhs) const noexcept { return !(*this == rhs); }
};

// Default permissions for newly-created objects
// Files:       rwd | rw- | r--   (7 | 6 | 4)
// Directories: rwx | r-x | r-x   (7 | 5 | 5)  — group/others can traverse but not write
inline constexpr Permission kDefaultFilePerms  = { PERM_ALL, PERM_READ | PERM_WRITE, PERM_READ };
inline constexpr Permission kDefaultDirPerms   = { PERM_ALL, PERM_READ | PERM_DELETE, PERM_READ | PERM_DELETE };

// Human-readable helpers
std::string permBitsToString(uint8_t bits, bool isDir = false);
std::string permissionToString(const Permission& p, bool isDir = false);
// Parse "640" / "750" etc. into a Permission; returns nullopt on invalid input
std::optional<Permission> parseOctalPermission(const std::string& str);

// ─────────────────────────────────────────
// Represents a user in the mock FS
// ─────────────────────────────────────────
struct User {
    std::string name;
    std::string group;
    bool        isRoot = false;

    User() = default;
    User(std::string name, std::string group, bool isRoot = false)
        : name(std::move(name)), group(std::move(group)), isRoot(isRoot) {}

    bool valid() const noexcept { return !name.empty(); }
};

// ─────────────────────────────────────────
// Supported file types in the mock FS
// ─────────────────────────────────────────
enum class FileType { Text, Csv, Unknown };

std::string fileTypeToString(FileType type);
FileType    inferFileTypeFromExtension(const std::string& extension);
bool        isSupportedFileExtension(const std::string& extension);

// ─────────────────────────────────────────
// Represents a single file in the FS
// ─────────────────────────────────────────
struct File {
    std::string name;
    std::string baseName;
    std::string extension;
    FileType    type;
    std::string content;
    std::string password;
    bool        locked;

    // Ownership & permissions
    std::string owner;
    std::string group;
    Permission  perms;

    File();
    File(const std::string& name,
         const std::string& content,
         const std::string& password = "",
         const std::string& owner    = "root",
         const std::string& group    = "root",
         Permission         perms    = kDefaultFilePerms);
};

// ─────────────────────────────────────────
// Represents a directory node
// ─────────────────────────────────────────
struct Directory {
    std::string name;
    std::unordered_map<std::string, File>      files;
    std::unordered_map<std::string, Directory> subdirs;

    // Ownership & permissions
    std::string owner;
    std::string group;
    Permission  perms;

    Directory() = default;
    explicit Directory(const std::string& name,
                       const std::string& owner = "root",
                       const std::string& group = "root",
                       Permission         perms = kDefaultDirPerms);
};

// ─────────────────────────────────────────
// Mock Filesystem
// ─────────────────────────────────────────
class FileSystem {
public:
    FileSystem();

    // ── Navigation ──────────────────────────────────────────────────────────
    bool        changeDirectory(const std::string& path);
    std::string currentPath() const;
    void        listDirectory() const;

    // ── File operations ──────────────────────────────────────────────────────
    bool createFile(const std::string& name,
                    const std::string& content,
                    const std::string& password = "");
    bool createTextFile(const std::string& name,
                        const std::string& content,
                        const std::string& password = "");
    bool createCsvFile(const std::string& name,
                       const std::string& content,
                       const std::string& password = "");
    bool displayFile(const std::string& name, const std::string& password = "");
    bool deleteFile(const std::string& name,  const std::string& password = "");
    bool editFile(const std::string& name,
                  const std::string& newContent,
                  const std::string& password = "");
    bool setPassword(const std::string& name,
                     const std::string& oldPassword,
                     const std::string& newPassword);
    bool removePassword(const std::string& name, const std::string& password);

    // ── Permission operations ─────────────────────────────────────────────────
    // Change permissions on a file in the current directory.
    // Only the file's owner or root may do this.
    // Each bit field must be in [0, 7]; returns false on invalid input or
    // if the caller lacks authority.
    bool chmod(const std::string& name,
               uint8_t ownerBits, uint8_t groupBits, uint8_t otherBits);

    // Change permissions on a subdirectory of the current directory.
    // Only the directory's owner or root may do this.
    bool chmodDir(const std::string& name,
                  uint8_t ownerBits, uint8_t groupBits, uint8_t otherBits);

    // Change ownership of a file. Root only.
    // newGroup may be empty to leave group unchanged.
    bool chown(const std::string& name,
               const std::string& newOwner,
               const std::string& newGroup = "");

    // Change ownership of a subdirectory. Root only.
    bool chownDir(const std::string& name,
                  const std::string& newOwner,
                  const std::string& newGroup = "");

    // Query the effective permission bits the current user has on a named file.
    // Returns 0 if the file does not exist.
    uint8_t effectiveFileBits(const std::string& name) const;

    // Query effective bits on a named subdirectory of the current directory.
    uint8_t effectiveDirBitsQuery(const std::string& name) const;

    // Convenience permission predicates (return false if file not found)
    bool canRead(const std::string& name)   const;
    bool canWrite(const std::string& name)  const;
    bool canDelete(const std::string& name) const;

    // ── User management ───────────────────────────────────────────────────────
    // Add a user to the filesystem's user registry.
    // Shell-level gate (useradd) enforces root-only; the API does not, to
    // allow fuzz targets to configure arbitrary scenarios.
    bool addUser(const std::string& name,
                 const std::string& group,
                 bool isRoot = false);

    // Switch the active user context (no password required in this mock).
    bool switchUser(const std::string& name);

    // Return the current user's name.
    const std::string& currentUserName() const;
    const User&        currentUser()     const;

    // ── Directory operations ──────────────────────────────────────────────────
    bool makeDirectory(const std::string& name);
    bool removeDirectory(const std::string& name);

    // ── Misc ─────────────────────────────────────────────────────────────────
    void printHelp() const;

private:
    Directory                root_;
    std::vector<std::string> pathStack_;

    std::unordered_map<std::string, User> users_;
    User                                  currentUser_;

    // Internal navigation
    Directory*       currentDir();
    const Directory* currentDir() const;
    std::string      pathString() const;

    // Authentication
    bool authenticate(const File& file, const std::string& password) const;

    // Permission engine
    uint8_t effectiveBits   (const File&      file) const;
    uint8_t effectiveDirBits(const Directory& dir)  const;
    bool    checkFilePerm   (const File&      file, uint8_t required) const;
    bool    checkDirPerm    (const Directory& dir,  uint8_t required) const;
};

// ─────────────────────────────────────────
// CLI shell loop
// ─────────────────────────────────────────
void runShell(FileSystem& fs, std::istream& in = std::cin);