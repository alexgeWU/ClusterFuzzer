#include "filesystem.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

// ─── anonymous helpers ────────────────────────────────────────────────────────
namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool isValidNamePart(const std::string& s) {
    if (s.empty()) return false;
    if (s == "." || s == "..") return false;
    return s.find('/') == std::string::npos && s.find('\\') == std::string::npos;
}

std::optional<std::pair<std::string, std::string>> splitFileName(const std::string& name) {
    const auto dot = name.find_last_of('.');
    if (dot == std::string::npos || dot == 0 || dot == name.size() - 1)
        return std::nullopt;
    std::string base = name.substr(0, dot);
    std::string ext  = toLower(name.substr(dot + 1));
    if (!isValidNamePart(base) || !isValidNamePart(ext)) return std::nullopt;
    return std::make_pair(base, ext);
}

std::string readPassword(const std::string& prompt, std::istream& in) {
    std::cout << prompt;
    std::string pw;
    std::getline(in, pw);
    return pw;
}

std::string readMultilineContent(const std::string& filename, std::istream& in) {
    std::cout << "Enter content for '" << filename
              << "' (type END on a new line to finish):\n";
    std::string content, line;
    while (std::getline(in, line)) {
        if (line == "END") break;
        content += line + "\n";
    }
    return content;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream ss(line);
    std::string tok;
    while (ss >> tok) tokens.push_back(tok);
    return tokens;
}

// ── CSV rendering (unchanged) ─────────────────────────────────────────────────
std::vector<std::string> parseCsvRow(const std::string& line) {
    std::vector<std::string> cells;
    std::string cell;
    bool inQuotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') { cell += '"'; ++i; }
            else inQuotes = !inQuotes;
        } else if (ch == ',' && !inQuotes) { cells.push_back(cell); cell.clear(); }
        else cell += ch;
    }
    cells.push_back(cell);
    return cells;
}

std::vector<std::vector<std::string>> parseCsv(const std::string& content) {
    std::vector<std::vector<std::string>> rows;
    std::istringstream input(content);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        rows.push_back(parseCsvRow(line));
    }
    return rows;
}

std::string truncateCell(const std::string& cell, std::size_t width) {
    if (cell.size() <= width) return cell;
    if (width <= 3) return cell.substr(0, width);
    return cell.substr(0, width - 3) + "...";
}

void displayCsvTable(const std::string& content) {
    const auto rows = parseCsv(content);
    if (rows.empty()) { std::cout << "(empty csv)\n"; return; }
    std::size_t cols = 0;
    for (const auto& row : rows) cols = std::max(cols, row.size());
    if (cols == 0) { std::cout << "(empty csv)\n"; return; }
    constexpr std::size_t maxWidth = 24;
    std::vector<std::size_t> widths(cols, 1);
    for (const auto& row : rows)
        for (std::size_t c = 0; c < row.size(); ++c)
            widths[c] = std::min(maxWidth, std::max(widths[c], row[c].size()));
    auto printSep = [&]() {
        std::cout << '+';
        for (std::size_t w : widths) std::cout << std::string(w + 2, '-') << '+';
        std::cout << '\n';
    };
    printSep();
    for (std::size_t r = 0; r < rows.size(); ++r) {
        std::cout << '|';
        for (std::size_t c = 0; c < cols; ++c) {
            std::string v = (c < rows[r].size()) ? truncateCell(rows[r][c], widths[c]) : "";
            std::cout << ' ' << std::left << std::setw(static_cast<int>(widths[c])) << v << std::right << '|';
        }
        std::cout << '\n';
        if (r == 0) printSep();
    }
    printSep();
}

} // namespace
// ─────────────────────────────────────────────────────────────────────────────

// ── Permission string helpers ─────────────────────────────────────────────────
std::string permBitsToString(uint8_t bits, bool isDir) {
    char lastChar = isDir ? 'x' : 'd';
    return {
        static_cast<char>((bits & PERM_READ)   ? 'r' : '-'),
        static_cast<char>((bits & PERM_WRITE)  ? 'w' : '-'),
        static_cast<char>((bits & PERM_DELETE) ? lastChar : '-')
    };
}

std::string permissionToString(const Permission& p, bool isDir) {
    return permBitsToString(p.owner, isDir) + "|"
         + permBitsToString(p.group, isDir) + "|"
         + permBitsToString(p.other, isDir);
}

std::optional<Permission> parseOctalPermission(const std::string& str) {
    if (str.size() != 3) return std::nullopt;
    for (char c : str)
        if (c < '0' || c > '7') return std::nullopt;
    return Permission::fromOctal(
        static_cast<uint8_t>(str[0] - '0'),
        static_cast<uint8_t>(str[1] - '0'),
        static_cast<uint8_t>(str[2] - '0'));
}

// ── FileType ──────────────────────────────────────────────────────────────────
std::string fileTypeToString(FileType type) {
    switch (type) {
        case FileType::Text:    return "text";
        case FileType::Csv:     return "csv";
        default:                return "unknown";
    }
}

FileType inferFileTypeFromExtension(const std::string& extension) {
    const std::string ext = toLower(extension);
    if (ext == "txt") return FileType::Text;
    if (ext == "csv") return FileType::Csv;
    return FileType::Unknown;
}

bool isSupportedFileExtension(const std::string& extension) {
    return inferFileTypeFromExtension(extension) != FileType::Unknown;
}

// ── File ─────────────────────────────────────────────────────────────────────
File::File()
    : type(FileType::Unknown), locked(false), perms(kDefaultFilePerms) {}

File::File(const std::string& name,
           const std::string& content,
           const std::string& password,
           const std::string& owner,
           const std::string& group,
           Permission         perms)
    : name(name), content(content), password(password),
      locked(!password.empty()), owner(owner), group(group), perms(perms) {
    auto parts = splitFileName(name);
    if (parts) {
        baseName  = parts->first;
        extension = parts->second;
        type      = inferFileTypeFromExtension(extension);
    } else {
        baseName = name;
        extension.clear();
        type = FileType::Unknown;
    }
}

// ── Directory ─────────────────────────────────────────────────────────────────
Directory::Directory(const std::string& name,
                     const std::string& owner,
                     const std::string& group,
                     Permission         perms)
    : name(name), owner(owner), group(group), perms(perms) {}

// ── FileSystem internals ──────────────────────────────────────────────────────
Directory* FileSystem::currentDir() {
    Directory* dir = &root_;
    for (const auto& seg : pathStack_) {
        auto it = dir->subdirs.find(seg);
        if (it == dir->subdirs.end()) return nullptr;
        dir = &it->second;
    }
    return dir;
}

const Directory* FileSystem::currentDir() const {
    const Directory* dir = &root_;
    for (const auto& seg : pathStack_) {
        auto it = dir->subdirs.find(seg);
        if (it == dir->subdirs.end()) return nullptr;
        dir = &it->second;
    }
    return dir;
}

std::string FileSystem::pathString() const {
    if (pathStack_.empty()) return "/";
    std::string p;
    for (const auto& s : pathStack_) p += "/" + s;
    return p;
}

bool FileSystem::authenticate(const File& file, const std::string& password) const {
    if (!file.locked) return true;
    return file.password == password;
}

// ── Permission engine ─────────────────────────────────────────────────────────

// Resolve which permission tier applies and return the raw bit mask.
// Root always receives PERM_ALL regardless of stored bits.
uint8_t FileSystem::effectiveBits(const File& file) const {
    if (currentUser_.isRoot)              return PERM_ALL;
    if (currentUser_.name  == file.owner) return file.perms.owner;
    if (currentUser_.group == file.group) return file.perms.group;
    return file.perms.other;
}

uint8_t FileSystem::effectiveDirBits(const Directory& dir) const {
    if (currentUser_.isRoot)             return PERM_ALL;
    if (currentUser_.name  == dir.owner) return dir.perms.owner;
    if (currentUser_.group == dir.group) return dir.perms.group;
    return dir.perms.other;
}

bool FileSystem::checkFilePerm(const File& file, uint8_t required) const {
    return (effectiveBits(file) & required) == required;
}

bool FileSystem::checkDirPerm(const Directory& dir, uint8_t required) const {
    return (effectiveDirBits(dir) & required) == required;
}

// ── Public API ────────────────────────────────────────────────────────────────

FileSystem::FileSystem()
    : root_("/", "root", "root", kDefaultDirPerms) {
    users_.emplace("root", User("root", "root", /*isRoot=*/true));
    currentUser_ = users_.at("root");
}

const std::string& FileSystem::currentUserName() const { return currentUser_.name; }
const User&        FileSystem::currentUser()     const { return currentUser_; }
std::string        FileSystem::currentPath()     const { return pathString(); }

// ── User management ───────────────────────────────────────────────────────────

bool FileSystem::addUser(const std::string& name,
                         const std::string& group,
                         bool               isRoot) {
    if (name.empty() || !isValidNamePart(name)) {
        std::cout << "Error: invalid username '" << name << "'.\n";
        return false;
    }
    if (users_.count(name)) {
        std::cout << "Error: user '" << name << "' already exists.\n";
        return false;
    }
    users_.emplace(name, User(name, group.empty() ? name : group, isRoot));
    std::cout << "User added: " << name << " (group=" << (group.empty() ? name : group) << ")\n";
    return true;
}

bool FileSystem::switchUser(const std::string& name) {
    auto it = users_.find(name);
    if (it == users_.end()) {
        std::cout << "Error: unknown user '" << name << "'.\n";
        return false;
    }
    currentUser_ = it->second;
    std::cout << "Switched to user: " << name << "\n";
    return true;
}

// ── Navigation ────────────────────────────────────────────────────────────────

bool FileSystem::changeDirectory(const std::string& path) {
    if (path == "/") { pathStack_.clear(); return true; }
    if (path == "..") {
        if (!pathStack_.empty()) pathStack_.pop_back();
        return true;
    }

    Directory* dir = currentDir();
    if (!dir) return false;

    // Traverse permission required on the current directory
    if (!checkDirPerm(*dir, PERM_DELETE)) {
        std::cout << "cd: permission denied: " << path << "\n";
        return false;
    }

    auto it = dir->subdirs.find(path);
    if (it == dir->subdirs.end()) {
        std::cout << "cd: no such directory: " << path << "\n";
        return false;
    }

    // Traverse permission required on the target directory
    if (!checkDirPerm(it->second, PERM_DELETE)) {
        std::cout << "cd: permission denied: " << path << "\n";
        return false;
    }

    pathStack_.push_back(path);
    return true;
}

void FileSystem::listDirectory() const {
    const Directory* dir = currentDir();
    if (!dir) { std::cout << "Error: invalid directory.\n"; return; }

    // Read permission required to list directory
    if (!checkDirPerm(*dir, PERM_READ)) {
        std::cout << "ls: permission denied\n";
        return;
    }

    std::cout << "\n  " << pathString()
              << "  [owner=" << dir->owner << ":" << dir->group
              << "  " << permissionToString(dir->perms, /*isDir=*/true) << "]\n";
    std::cout << "  " << std::string(68, '-') << "\n";

    if (dir->subdirs.empty() && dir->files.empty()) std::cout << "  (empty)\n";

    for (const auto& [name, subdir] : dir->subdirs) {
        std::cout << "  [DIR]   " << std::left << std::setw(24) << name
                  << "  " << permissionToString(subdir.perms, true)
                  << "  " << subdir.owner << ":" << subdir.group << "\n";
    }
    for (const auto& [name, file] : dir->files) {
        std::string typeStr = file.type == FileType::Csv  ? "CSV  "
                            : file.type == FileType::Text ? "TEXT " : "FILE ";
        std::cout << "  [" << typeStr << "] " << std::left << std::setw(24) << name
                  << "  " << permissionToString(file.perms)
                  << "  " << file.owner << ":" << file.group;
        if (file.locked) std::cout << "  \xF0\x9F\x94\x92";
        std::cout << "\n";
    }
    std::cout << "\n";
}

// ── File creation ─────────────────────────────────────────────────────────────

bool FileSystem::createFile(const std::string& name,
                            const std::string& content,
                            const std::string& password) {
    auto parts = splitFileName(name);
    if (!parts) {
        std::cout << "Error: files must use a name with a .txt or .csv extension.\n";
        return false;
    }
    if (!isSupportedFileExtension(parts->second)) {
        std::cout << "Error: unsupported file extension '." << parts->second
                  << "'. Only .txt and .csv are allowed.\n";
        return false;
    }

    Directory* dir = currentDir();
    if (!dir) return false;

    // Write permission required on the containing directory
    if (!checkDirPerm(*dir, PERM_WRITE)) {
        std::cout << "Error: permission denied (directory write required).\n";
        return false;
    }

    if (dir->files.count(name)) {
        std::cout << "Error: file '" << name << "' already exists.\n";
        return false;
    }

    File file(name, content, password, currentUser_.name, currentUser_.group);
    dir->files.emplace(name, file);
    std::cout << "Created " << fileTypeToString(file.type) << " file: " << name;
    if (!password.empty()) std::cout << " (password protected)";
    std::cout << "\n";
    return true;
}

bool FileSystem::createTextFile(const std::string& name,
                                const std::string& content,
                                const std::string& password) {
    auto parts = splitFileName(name);
    if (!parts || parts->second != "txt") {
        std::cout << "Error: text files must use the .txt extension.\n";
        return false;
    }
    return createFile(name, content, password);
}

bool FileSystem::createCsvFile(const std::string& name,
                               const std::string& content,
                               const std::string& password) {
    auto parts = splitFileName(name);
    if (!parts || parts->second != "csv") {
        std::cout << "Error: CSV files must use the .csv extension.\n";
        return false;
    }
    return createFile(name, content, password);
}

// ── File access ───────────────────────────────────────────────────────────────

bool FileSystem::displayFile(const std::string& name, const std::string& password) {
    Directory* dir = currentDir();
    if (!dir) return false;

    auto it = dir->files.find(name);
    if (it == dir->files.end()) {
        std::cout << "Error: file '" << name << "' not found.\n";
        return false;
    }

    // 1. Permission check (before password — avoids leaking lock status to
    //    unpermitted callers).
    if (!checkFilePerm(it->second, PERM_READ)) {
        std::cout << "Error: permission denied.\n";
        return false;
    }

    // 2. Password check
    if (!authenticate(it->second, password)) {
        std::cout << "Error: incorrect password.\n";
        return false;
    }

    const File& file = it->second;
    std::cout << "\n── " << file.name
              << " [" << fileTypeToString(file.type) << ", ." << file.extension
              << "] ──────────────────────────\n";
    if (file.type == FileType::Csv) displayCsvTable(file.content);
    else std::cout << file.content << "\n";
    std::cout << "────────────────────────────────────────\n\n";
    return true;
}

bool FileSystem::deleteFile(const std::string& name, const std::string& password) {
    Directory* dir = currentDir();
    if (!dir) return false;

    auto it = dir->files.find(name);
    if (it == dir->files.end()) {
        std::cout << "Error: file '" << name << "' not found.\n";
        return false;
    }

    if (!checkFilePerm(it->second, PERM_DELETE)) {
        std::cout << "Error: permission denied.\n";
        return false;
    }

    if (!authenticate(it->second, password)) {
        std::cout << "Error: incorrect password.\n";
        return false;
    }

    dir->files.erase(it);
    std::cout << "Deleted: " << name << "\n";
    return true;
}

bool FileSystem::editFile(const std::string& name,
                          const std::string& newContent,
                          const std::string& password) {
    Directory* dir = currentDir();
    if (!dir) return false;

    auto it = dir->files.find(name);
    if (it == dir->files.end()) {
        std::cout << "Error: file '" << name << "' not found.\n";
        return false;
    }

    if (!checkFilePerm(it->second, PERM_WRITE)) {
        std::cout << "Error: permission denied.\n";
        return false;
    }

    if (!authenticate(it->second, password)) {
        std::cout << "Error: incorrect password.\n";
        return false;
    }

    it->second.content = newContent;
    std::cout << "Updated: " << name << "\n";
    return true;
}

bool FileSystem::setPassword(const std::string& name,
                             const std::string& oldPassword,
                             const std::string& newPassword) {
    Directory* dir = currentDir();
    if (!dir) return false;

    auto it = dir->files.find(name);
    if (it == dir->files.end()) {
        std::cout << "Error: file '" << name << "' not found.\n";
        return false;
    }

    // Changing a password requires write permission on the file
    if (!checkFilePerm(it->second, PERM_WRITE)) {
        std::cout << "Error: permission denied.\n";
        return false;
    }

    if (!authenticate(it->second, oldPassword)) {
        std::cout << "Error: incorrect password.\n";
        return false;
    }

    it->second.password = newPassword;
    it->second.locked   = !newPassword.empty();
    std::cout << "Password updated for: " << name << "\n";
    return true;
}

bool FileSystem::removePassword(const std::string& name, const std::string& password) {
    return setPassword(name, password, "");
}

// ── Permission operations ─────────────────────────────────────────────────────

bool FileSystem::chmod(const std::string& name,
                       uint8_t ownerBits, uint8_t groupBits, uint8_t otherBits) {
    if (ownerBits > 7 || groupBits > 7 || otherBits > 7) {
        std::cout << "Error: permission bits must be in range [0, 7].\n";
        return false;
    }

    Directory* dir = currentDir();
    if (!dir) return false;

    auto it = dir->files.find(name);
    if (it == dir->files.end()) {
        std::cout << "Error: file '" << name << "' not found.\n";
        return false;
    }

    // Only the file's owner or root may change its permissions
    if (!currentUser_.isRoot && currentUser_.name != it->second.owner) {
        std::cout << "Error: only the file owner or root may change permissions.\n";
        return false;
    }

    it->second.perms = Permission::fromOctal(ownerBits, groupBits, otherBits);
    std::cout << "Permissions updated for '" << name << "': "
              << permissionToString(it->second.perms) << "\n";
    return true;
}

bool FileSystem::chmodDir(const std::string& name,
                          uint8_t ownerBits, uint8_t groupBits, uint8_t otherBits) {
    if (ownerBits > 7 || groupBits > 7 || otherBits > 7) {
        std::cout << "Error: permission bits must be in range [0, 7].\n";
        return false;
    }

    Directory* dir = currentDir();
    if (!dir) return false;

    auto it = dir->subdirs.find(name);
    if (it == dir->subdirs.end()) {
        std::cout << "Error: directory '" << name << "' not found.\n";
        return false;
    }

    if (!currentUser_.isRoot && currentUser_.name != it->second.owner) {
        std::cout << "Error: only the directory owner or root may change permissions.\n";
        return false;
    }

    it->second.perms = Permission::fromOctal(ownerBits, groupBits, otherBits);
    std::cout << "Directory permissions updated for '" << name << "': "
              << permissionToString(it->second.perms, /*isDir=*/true) << "\n";
    return true;
}

bool FileSystem::chown(const std::string& name,
                       const std::string& newOwner,
                       const std::string& newGroup) {
    if (!currentUser_.isRoot) {
        std::cout << "Error: only root may change file ownership.\n";
        return false;
    }

    auto uit = users_.find(newOwner);
    if (uit == users_.end()) {
        std::cout << "Error: unknown user '" << newOwner << "'.\n";
        return false;
    }

    Directory* dir = currentDir();
    if (!dir) return false;

    auto it = dir->files.find(name);
    if (it == dir->files.end()) {
        std::cout << "Error: file '" << name << "' not found.\n";
        return false;
    }

    it->second.owner = newOwner;
    if (!newGroup.empty()) it->second.group = newGroup;
    std::cout << "Ownership of '" << name << "' changed to " << newOwner;
    if (!newGroup.empty()) std::cout << ":" << newGroup;
    std::cout << "\n";
    return true;
}

bool FileSystem::chownDir(const std::string& name,
                          const std::string& newOwner,
                          const std::string& newGroup) {
    if (!currentUser_.isRoot) {
        std::cout << "Error: only root may change directory ownership.\n";
        return false;
    }

    auto uit = users_.find(newOwner);
    if (uit == users_.end()) {
        std::cout << "Error: unknown user '" << newOwner << "'.\n";
        return false;
    }

    Directory* dir = currentDir();
    if (!dir) return false;

    auto it = dir->subdirs.find(name);
    if (it == dir->subdirs.end()) {
        std::cout << "Error: directory '" << name << "' not found.\n";
        return false;
    }

    it->second.owner = newOwner;
    if (!newGroup.empty()) it->second.group = newGroup;
    std::cout << "Ownership of directory '" << name << "' changed to " << newOwner;
    if (!newGroup.empty()) std::cout << ":" << newGroup;
    std::cout << "\n";
    return true;
}

// ── Permission query helpers ──────────────────────────────────────────────────

uint8_t FileSystem::effectiveFileBits(const std::string& name) const {
    const Directory* dir = currentDir();
    if (!dir) return 0;
    auto it = dir->files.find(name);
    if (it == dir->files.end()) return 0;
    return effectiveBits(it->second);
}

uint8_t FileSystem::effectiveDirBitsQuery(const std::string& name) const {
    const Directory* dir = currentDir();
    if (!dir) return 0;
    auto it = dir->subdirs.find(name);
    if (it == dir->subdirs.end()) return 0;
    return effectiveDirBits(it->second);
}

bool FileSystem::canRead(const std::string& name)   const { return (effectiveFileBits(name) & PERM_READ)   != 0; }
bool FileSystem::canWrite(const std::string& name)  const { return (effectiveFileBits(name) & PERM_WRITE)  != 0; }
bool FileSystem::canDelete(const std::string& name) const { return (effectiveFileBits(name) & PERM_DELETE) != 0; }

// ── Directory operations ──────────────────────────────────────────────────────

bool FileSystem::makeDirectory(const std::string& name) {
    Directory* dir = currentDir();
    if (!dir) return false;

    if (!isValidNamePart(name) || name.find('.') != std::string::npos) {
        std::cout << "Error: invalid directory name. Directory names should not contain slashes or file extensions.\n";
        return false;
    }

    if (!checkDirPerm(*dir, PERM_WRITE)) {
        std::cout << "Error: permission denied (directory write required).\n";
        return false;
    }

    if (dir->subdirs.count(name)) {
        std::cout << "Error: directory '" << name << "' already exists.\n";
        return false;
    }

    dir->subdirs.emplace(name, Directory(name, currentUser_.name, currentUser_.group));
    std::cout << "Directory created: " << name << "\n";
    return true;
}

bool FileSystem::removeDirectory(const std::string& name) {
    Directory* dir = currentDir();
    if (!dir) return false;

    if (!checkDirPerm(*dir, PERM_WRITE)) {
        std::cout << "Error: permission denied (directory write required).\n";
        return false;
    }

    auto it = dir->subdirs.find(name);
    if (it == dir->subdirs.end()) {
        std::cout << "Error: directory '" << name << "' not found.\n";
        return false;
    }

    if (!it->second.files.empty() || !it->second.subdirs.empty()) {
        std::cout << "Error: directory '" << name << "' is not empty.\n";
        return false;
    }

    dir->subdirs.erase(it);
    std::cout << "Removed directory: " << name << "\n";
    return true;
}

// ── Help ──────────────────────────────────────────────────────────────────────

void FileSystem::printHelp() const {
    std::cout << R"(
  MockFS Commands
  ─────────────────────────────────────────────────────────────────────────
  ls                              List directory (requires dir read perm)
  cd <dir>                        Change directory (requires traverse perm)
  mkdir <dir>                     Create directory (requires dir write perm)
  rmdir <dir>                     Remove empty directory (dir write perm)
  pwd                             Print working directory

  touch <file.txt|file.csv>       Create empty file (requires dir write)
  write <file.txt>                Create text file interactively
  csv <file.csv>                  Create CSV file interactively
  cat   <file> [pw]               Display file   (requires read perm)
  edit  <file> [pw]               Edit file      (requires write perm)
  rm    <file> [pw]               Delete file    (requires delete perm)

  passwd   <file>                 Set/change password (requires write perm)
  unpasswd <file>                 Remove password     (requires write perm)

  chmod <ooo> <file>              Change file permissions (owner/root only)
  chmodd <ooo> <dir>              Change directory permissions (owner/root)
                                   e.g.  chmod 640 notes.txt
                                         o=6(rw-)  g=4(r--)  u=0(---)
  chown <user>[:<group>] <file>   Change file ownership       (root only)
  chownd <user>[:<group>] <dir>   Change directory ownership  (root only)

  whoami                          Show current user and group
  su <user>                       Switch user (no password in mock)
  useradd <name> [group]          Add user (root only at shell level)

  help                            Show this help
  exit                            Quit the shell
  ─────────────────────────────────────────────────────────────────────────
  Permission bits:  r=4  w=2  d/x=1
    File bits:      r=read  w=write  d=delete
    Dir  bits:      r=list  w=create/remove-entries  x=traverse(cd)
)";
}

// ── Shell loop ────────────────────────────────────────────────────────────────

void runShell(FileSystem& fs, std::istream& in) {
    std::cout << "\n  MockFS  —  type 'help' for commands\n\n";

    std::string line;
    while (true) {
        std::cout << "mockfs[" << fs.currentUserName() << "]:" << fs.currentPath() << "$ ";
        if (!std::getline(in, line)) break;

        auto args = tokenize(line);
        if (args.empty()) continue;

        const std::string& cmd = args[0];

        if (cmd == "exit" || cmd == "quit") {
            std::cout << "Goodbye.\n";
            break;

        } else if (cmd == "help") {
            fs.printHelp();

        } else if (cmd == "pwd") {
            std::cout << fs.currentPath() << "\n";

        } else if (cmd == "whoami") {
            const User& u = fs.currentUser();
            std::cout << u.name << " (group=" << u.group << ")"
                      << (u.isRoot ? " [root]" : "") << "\n";

        } else if (cmd == "su") {
            if (args.size() < 2) { std::cout << "Usage: su <user>\n"; continue; }
            fs.switchUser(args[1]);

        } else if (cmd == "useradd") {
            if (args.size() < 2) { std::cout << "Usage: useradd <name> [group]\n"; continue; }
            if (!fs.currentUser().isRoot) {
                std::cout << "Error: only root may add users.\n"; continue;
            }
            std::string grp = (args.size() >= 3) ? args[2] : args[1];
            fs.addUser(args[1], grp);

        } else if (cmd == "chmod") {
            if (args.size() < 3) { std::cout << "Usage: chmod <ooo> <file>\n"; continue; }
            auto p = parseOctalPermission(args[1]);
            if (!p) { std::cout << "Error: invalid permission string '" << args[1] << "'.\n"; continue; }
            fs.chmod(args[2], p->owner, p->group, p->other);

        } else if (cmd == "chmodd") {
            if (args.size() < 3) { std::cout << "Usage: chmodd <ooo> <dir>\n"; continue; }
            auto p = parseOctalPermission(args[1]);
            if (!p) { std::cout << "Error: invalid permission string '" << args[1] << "'.\n"; continue; }
            fs.chmodDir(args[2], p->owner, p->group, p->other);

        } else if (cmd == "chown") {
            if (args.size() < 3) { std::cout << "Usage: chown <owner>[:<group>] <file>\n"; continue; }
            std::string ownerArg = args[1];
            std::string newGroup;
            auto colon = ownerArg.find(':');
            if (colon != std::string::npos) {
                newGroup = ownerArg.substr(colon + 1);
                ownerArg = ownerArg.substr(0, colon);
            }
            fs.chown(args[2], ownerArg, newGroup);

        } else if (cmd == "chownd") {
            if (args.size() < 3) { std::cout << "Usage: chownd <owner>[:<group>] <dir>\n"; continue; }
            std::string ownerArg = args[1];
            std::string newGroup;
            auto colon = ownerArg.find(':');
            if (colon != std::string::npos) {
                newGroup = ownerArg.substr(colon + 1);
                ownerArg = ownerArg.substr(0, colon);
            }
            fs.chownDir(args[2], ownerArg, newGroup);

        } else if (cmd == "ls") {
            fs.listDirectory();

        } else if (cmd == "cd") {
            if (args.size() < 2) { std::cout << "Usage: cd <dir>\n"; continue; }
            fs.changeDirectory(args[1]);

        } else if (cmd == "mkdir") {
            if (args.size() < 2) { std::cout << "Usage: mkdir <dir>\n"; continue; }
            fs.makeDirectory(args[1]);

        } else if (cmd == "rmdir") {
            if (args.size() < 2) { std::cout << "Usage: rmdir <dir>\n"; continue; }
            fs.removeDirectory(args[1]);

        } else if (cmd == "touch") {
            if (args.size() < 2) { std::cout << "Usage: touch <file.txt|file.csv>\n"; continue; }
            fs.createFile(args[1], "");

        } else if (cmd == "write") {
            if (args.size() < 2) { std::cout << "Usage: write <file.txt>\n"; continue; }
            std::string content = readMultilineContent(args[1], in);
            std::cout << "Password protect? (y/n): ";
            std::string answer; std::getline(in, answer);
            std::string pw;
            if (!answer.empty() && (answer[0] == 'y' || answer[0] == 'Y'))
                pw = readPassword("Set password: ", in);
            fs.createTextFile(args[1], content, pw);

        } else if (cmd == "csv") {
            if (args.size() < 2) { std::cout << "Usage: csv <file.csv>\n"; continue; }
            std::string content = readMultilineContent(args[1], in);
            std::cout << "Password protect? (y/n): ";
            std::string answer; std::getline(in, answer);
            std::string pw;
            if (!answer.empty() && (answer[0] == 'y' || answer[0] == 'Y'))
                pw = readPassword("Set password: ", in);
            fs.createCsvFile(args[1], content, pw);

        } else if (cmd == "cat") {
            if (args.size() < 2) { std::cout << "Usage: cat <file> [password]\n"; continue; }
            std::string pw = (args.size() >= 3) ? args[2]
                                                 : readPassword("Password (blank if none): ", in);
            fs.displayFile(args[1], pw);

        } else if (cmd == "edit") {
            if (args.size() < 2) { std::cout << "Usage: edit <file>\n"; continue; }
            std::string pw = (args.size() >= 3) ? args[2]
                                                 : readPassword("Password (blank if none): ", in);
            std::string content = readMultilineContent(args[1], in);
            fs.editFile(args[1], content, pw);

        } else if (cmd == "rm") {
            if (args.size() < 2) { std::cout << "Usage: rm <file>\n"; continue; }
            std::string pw = (args.size() >= 3) ? args[2]
                                                 : readPassword("Password (blank if none): ", in);
            fs.deleteFile(args[1], pw);

        } else if (cmd == "passwd") {
            if (args.size() < 2) { std::cout << "Usage: passwd <file>\n"; continue; }
            std::string oldPw     = readPassword("Current password (blank if none): ", in);
            std::string newPw     = readPassword("New password: ", in);
            std::string confirmPw = readPassword("Confirm new password: ", in);
            if (newPw != confirmPw) { std::cout << "Error: passwords do not match.\n"; continue; }
            fs.setPassword(args[1], oldPw, newPw);

        } else if (cmd == "unpasswd") {
            if (args.size() < 2) { std::cout << "Usage: unpasswd <file>\n"; continue; }
            std::string pw = readPassword("Current password: ", in);
            fs.removePassword(args[1], pw);

        } else {
            std::cout << "Unknown command: '" << cmd << "'. Type 'help' for commands.\n";
        }
    }
}