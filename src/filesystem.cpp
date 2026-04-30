#include "filesystem.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>

// ══════════════════════════════════════════
// File
// ══════════════════════════════════════════

File::File(const std::string& name,
           const std::string& content,
           const std::string& password)
    : name(name), content(content), password(password),
      locked(!password.empty()) {}

// ══════════════════════════════════════════
// Directory
// ══════════════════════════════════════════

Directory::Directory(const std::string& name) : name(name) {}

// ══════════════════════════════════════════
// FileSystem — private helpers
// ══════════════════════════════════════════

Directory* FileSystem::currentDir() {
    Directory* dir = &root_;
    for (const auto& segment : pathStack_) {
        auto it = dir->subdirs.find(segment);
        if (it == dir->subdirs.end()) return nullptr;
        dir = &it->second;
    }
    return dir;
}

const Directory* FileSystem::currentDir() const {
    const Directory* dir = &root_;
    for (const auto& segment : pathStack_) {
        auto it = dir->subdirs.find(segment);
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

// ══════════════════════════════════════════
// FileSystem — public interface
// ══════════════════════════════════════════

FileSystem::FileSystem() : root_("/") {}

std::string FileSystem::currentPath() const { return pathString(); }

// ── Navigation ────────────────────────────

bool FileSystem::changeDirectory(const std::string& path) {
    if (path == "/") {
        pathStack_.clear();
        return true;
    }
    if (path == "..") {
        if (!pathStack_.empty()) pathStack_.pop_back();
        return true;
    }

    Directory* dir = currentDir();
    if (!dir) return false;

    if (dir->subdirs.count(path)) {
        pathStack_.push_back(path);
        return true;
    }

    std::cout << "cd: no such directory: " << path << "\n";
    return false;
}

void FileSystem::listDirectory() const {
    const Directory* dir = currentDir();
    if (!dir) { std::cout << "Error: invalid directory.\n"; return; }

    std::cout << "\n  " << pathString() << "\n";
    std::cout << "  " << std::string(40, '-') << "\n";

    if (dir->subdirs.empty() && dir->files.empty()) {
        std::cout << "  (empty)\n";
    }

    for (const auto& [name, subdir] : dir->subdirs)
        std::cout << "  [DIR]  " << name << "\n";

    for (const auto& [name, file] : dir->files) {
        std::cout << "  [FILE] " << name;
        if (file.locked) std::cout << "  \xF0\x9F\x94\x92"; // 🔒 UTF-8
        std::cout << "\n";
    }
    std::cout << "\n";
}

// ── File operations ───────────────────────

bool FileSystem::createFile(const std::string& name,
                             const std::string& content,
                             const std::string& password) {
    Directory* dir = currentDir();
    if (!dir) return false;

    if (dir->files.count(name)) {
        std::cout << "Error: file '" << name << "' already exists.\n";
        return false;
    }

    dir->files.emplace(name, File(name, content, password));
    std::cout << "Created: " << name;
    if (!password.empty()) std::cout << " (password protected)";
    std::cout << "\n";
    return true;
}

bool FileSystem::displayFile(const std::string& name,
                              const std::string& password) {
    Directory* dir = currentDir();
    if (!dir) return false;

    auto it = dir->files.find(name);
    if (it == dir->files.end()) {
        std::cout << "Error: file '" << name << "' not found.\n";
        return false;
    }

    if (!authenticate(it->second, password)) {
        std::cout << "Error: incorrect password.\n";
        return false;
    }

    std::cout << "\n── " << name << " ──────────────────────────\n";
    std::cout << it->second.content << "\n";
    std::cout << "────────────────────────────────────────\n\n";
    return true;
}

bool FileSystem::deleteFile(const std::string& name,
                             const std::string& password) {
    Directory* dir = currentDir();
    if (!dir) return false;

    auto it = dir->files.find(name);
    if (it == dir->files.end()) {
        std::cout << "Error: file '" << name << "' not found.\n";
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

    if (!authenticate(it->second, oldPassword)) {
        std::cout << "Error: incorrect password.\n";
        return false;
    }

    it->second.password = newPassword;
    it->second.locked   = !newPassword.empty();
    std::cout << "Password updated for: " << name << "\n";
    return true;
}

bool FileSystem::removePassword(const std::string& name,
                                 const std::string& password) {
    return setPassword(name, password, "");
}

// ── Directory operations ──────────────────

bool FileSystem::makeDirectory(const std::string& name) {
    Directory* dir = currentDir();
    if (!dir) return false;

    if (dir->subdirs.count(name)) {
        std::cout << "Error: directory '" << name << "' already exists.\n";
        return false;
    }

    dir->subdirs.emplace(name, Directory(name));
    std::cout << "Directory created: " << name << "\n";
    return true;
}

bool FileSystem::removeDirectory(const std::string& name) {
    Directory* dir = currentDir();
    if (!dir) return false;

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

// ── Help ──────────────────────────────────

void FileSystem::printHelp() const {
    std::cout << R"(
  MockFS Commands
  ────────────────────────────────────────────────────────
  ls                             List current directory
  cd <dir>                       Change directory (use .. to go up)
  mkdir <dir>                    Create a directory
  rmdir <dir>                    Remove an empty directory
  pwd                            Print working directory

  touch <file>                   Create an empty file
  write <file>                   Create a file (enter content interactively)
  cat <file> [password]          Display file contents
  edit <file> [password]         Edit file content interactively
  rm <file> [password]           Delete a file

  passwd <file>                  Set or change a file's password
  unpasswd <file> <password>     Remove password from a file

  help                           Show this help
  exit                           Quit the shell
  ────────────────────────────────────────────────────────
)";
}

// ══════════════════════════════════════════
// Shell — helpers
// ══════════════════════════════════════════

static std::string readPassword(const std::string& prompt) {
    std::cout << prompt;
    std::string pw;
    std::getline(std::cin, pw);
    return pw;
}

static std::string readMultilineContent(const std::string& filename) {
    std::cout << "Enter content for '" << filename
              << "' (type END on a new line to finish):\n";
    std::string content, line;
    while (std::getline(std::cin, line)) {
        if (line == "END") break;
        content += line + "\n";
    }
    return content;
}

static std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream ss(line);
    std::string tok;
    while (ss >> tok) tokens.push_back(tok);
    return tokens;
}

// ══════════════════════════════════════════
// Shell — main loop
// ══════════════════════════════════════════

void runShell(FileSystem& fs) {
    std::cout << "\n  MockFS  —  type 'help' for commands\n\n";

    std::string line;
    while (true) {
        std::cout << "mockfs:" << fs.currentPath() << "$ ";
        if (!std::getline(std::cin, line)) break;   // EOF

        auto args = tokenize(line);
        if (args.empty()) continue;

        const std::string& cmd = args[0];

        // ── exit ──────────────────────────────
        if (cmd == "exit" || cmd == "quit") {
            std::cout << "Goodbye.\n";
            break;
        }

        // ── help ──────────────────────────────
        else if (cmd == "help") {
            fs.printHelp();
        }

        // ── pwd ───────────────────────────────
        else if (cmd == "pwd") {
            std::cout << fs.currentPath() << "\n";
        }

        // ── ls ────────────────────────────────
        else if (cmd == "ls") {
            fs.listDirectory();
        }

        // ── cd ────────────────────────────────
        else if (cmd == "cd") {
            if (args.size() < 2) { std::cout << "Usage: cd <dir>\n"; continue; }
            fs.changeDirectory(args[1]);
        }

        // ── mkdir ─────────────────────────────
        else if (cmd == "mkdir") {
            if (args.size() < 2) { std::cout << "Usage: mkdir <dir>\n"; continue; }
            fs.makeDirectory(args[1]);
        }

        // ── rmdir ─────────────────────────────
        else if (cmd == "rmdir") {
            if (args.size() < 2) { std::cout << "Usage: rmdir <dir>\n"; continue; }
            fs.removeDirectory(args[1]);
        }

        // ── touch (empty file) ────────────────
        else if (cmd == "touch") {
            if (args.size() < 2) { std::cout << "Usage: touch <file>\n"; continue; }
            fs.createFile(args[1], "");
        }

        // ── write (file with content) ─────────
        else if (cmd == "write") {
            if (args.size() < 2) { std::cout << "Usage: write <file>\n"; continue; }
            std::string content = readMultilineContent(args[1]);
            std::string pw;
            std::cout << "Password protect? (y/n): ";
            char c; std::cin >> c; std::cin.ignore();
            if (c == 'y' || c == 'Y') pw = readPassword("Set password: ");
            fs.createFile(args[1], content, pw);
        }

        // ── cat ───────────────────────────────
        else if (cmd == "cat") {
            if (args.size() < 2) { std::cout << "Usage: cat <file> [password]\n"; continue; }
            std::string pw = (args.size() >= 3) ? args[2] : "";

            // prompt for password if file exists but no pw given
            if (pw.empty()) {
                std::cout << "Password (leave blank if none): ";
                std::getline(std::cin, pw);
            }
            fs.displayFile(args[1], pw);
        }

        // ── edit ──────────────────────────────
        else if (cmd == "edit") {
            if (args.size() < 2) { std::cout << "Usage: edit <file>\n"; continue; }
            std::string pw;
            std::cout << "Password (leave blank if none): ";
            std::getline(std::cin, pw);
            std::string content = readMultilineContent(args[1]);
            fs.editFile(args[1], content, pw);
        }

        // ── rm ────────────────────────────────
        else if (cmd == "rm") {
            if (args.size() < 2) { std::cout << "Usage: rm <file>\n"; continue; }
            std::string pw;
            std::cout << "Password (leave blank if none): ";
            std::getline(std::cin, pw);
            fs.deleteFile(args[1], pw);
        }

        // ── passwd ────────────────────────────
        else if (cmd == "passwd") {
            if (args.size() < 2) { std::cout << "Usage: passwd <file>\n"; continue; }
            std::string oldPw = readPassword("Current password (blank if none): ");
            std::string newPw = readPassword("New password: ");
            std::string confirmPw = readPassword("Confirm new password: ");
            if (newPw != confirmPw) { std::cout << "Error: passwords do not match.\n"; continue; }
            fs.setPassword(args[1], oldPw, newPw);
        }

        // ── unpasswd ──────────────────────────
        else if (cmd == "unpasswd") {
            if (args.size() < 2) { std::cout << "Usage: unpasswd <file>\n"; continue; }
            std::string pw = readPassword("Current password: ");
            fs.removePassword(args[1], pw);
        }

        // ── unknown ───────────────────────────
        else {
            std::cout << "Unknown command: '" << cmd << "'. Type 'help' for commands.\n";
        }
    }
}