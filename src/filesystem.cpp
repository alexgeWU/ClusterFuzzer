#include "filesystem.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

bool isValidNamePart(const std::string& s) {
    if (s.empty()) return false;
    if (s == "." || s == "..") return false;
    return s.find('/') == std::string::npos && s.find('\\') == std::string::npos;
}

std::optional<std::pair<std::string, std::string>> splitFileName(const std::string& name) {
    const auto dot = name.find_last_of('.');
    if (dot == std::string::npos || dot == 0 || dot == name.size() - 1) {
        return std::nullopt;
    }

    std::string base = name.substr(0, dot);
    std::string ext = toLower(name.substr(dot + 1));
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

std::vector<std::string> parseCsvRow(const std::string& line) {
    std::vector<std::string> cells;
    std::string cell;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                cell += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            cells.push_back(cell);
            cell.clear();
        } else {
            cell += ch;
        }
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
    if (rows.empty()) {
        std::cout << "(empty csv)\n";
        return;
    }

    std::size_t cols = 0;
    for (const auto& row : rows) cols = std::max(cols, row.size());
    if (cols == 0) {
        std::cout << "(empty csv)\n";
        return;
    }

    constexpr std::size_t maxWidth = 24;
    std::vector<std::size_t> widths(cols, 1);
    for (const auto& row : rows) {
        for (std::size_t c = 0; c < row.size(); ++c) {
            widths[c] = std::min(maxWidth, std::max(widths[c], row[c].size()));
        }
    }

    auto printSeparator = [&]() {
        std::cout << '+';
        for (std::size_t w : widths) std::cout << std::string(w + 2, '-') << '+';
        std::cout << '\n';
    };

    printSeparator();
    for (std::size_t r = 0; r < rows.size(); ++r) {
        std::cout << '|';
        for (std::size_t c = 0; c < cols; ++c) {
            std::string value = (c < rows[r].size()) ? truncateCell(rows[r][c], widths[c]) : "";
            std::cout << ' ' << std::left << std::setw(static_cast<int>(widths[c])) << value << std::right << '|' ;
        }
        std::cout << '\n';
        if (r == 0) printSeparator();
    }
    printSeparator();
}

} // namespace

std::string fileTypeToString(FileType type) {
    switch (type) {
        case FileType::Text: return "text";
        case FileType::Csv: return "csv";
        default: return "unknown";
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

File::File()
    : type(FileType::Unknown), locked(false) {}

File::File(const std::string& name,
           const std::string& content,
           const std::string& password)
    : name(name), content(content), password(password), locked(!password.empty()) {
    auto parts = splitFileName(name);
    if (parts) {
        baseName = parts->first;
        extension = parts->second;
        type = inferFileTypeFromExtension(extension);
    } else {
        baseName = name;
        extension.clear();
        type = FileType::Unknown;
    }
}

Directory::Directory(const std::string& name) : name(name) {}

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

Directory* FileSystem::navigateTo(const std::string&) { return currentDir(); }

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

FileSystem::FileSystem() : root_("/") {}

std::string FileSystem::currentPath() const { return pathString(); }

bool FileSystem::changeDirectory(const std::string& path) {
    if (path == "/") { pathStack_.clear(); return true; }
    if (path == "..") { if (!pathStack_.empty()) pathStack_.pop_back(); return true; }

    Directory* dir = currentDir();
    if (!dir) return false;

    if (dir->subdirs.count(path)) { pathStack_.push_back(path); return true; }

    std::cout << "cd: no such directory: " << path << "\n";
    return false;
}

void FileSystem::listDirectory() const {
    const Directory* dir = currentDir();
    if (!dir) { std::cout << "Error: invalid directory.\n"; return; }

    std::cout << "\n  " << pathString() << "\n";
    std::cout << "  " << std::string(52, '-') << "\n";

    if (dir->subdirs.empty() && dir->files.empty()) std::cout << "  (empty)\n";

    for (const auto& [name, subdir] : dir->subdirs) {
        (void)subdir;
        std::cout << "  [DIR]   " << name << "\n";
    }

    for (const auto& [name, file] : dir->files) {
        std::cout << "  [" << (file.type == FileType::Csv ? "CSV  " : file.type == FileType::Text ? "TEXT " : "FILE ")
                  << "] " << name << "  ." << file.extension;
        if (file.locked) std::cout << "  \xF0\x9F\x94\x92";
        std::cout << "\n";
    }
    std::cout << "\n";
}

bool FileSystem::createFile(const std::string& name,
                            const std::string& content,
                            const std::string& password) {
    auto parts = splitFileName(name);
    if (!parts) {
        std::cout << "Error: files must use a name with a .txt or .csv extension.\n";
        return false;
    }

    if (!isSupportedFileExtension(parts->second)) {
        std::cout << "Error: unsupported file extension '." << parts->second << "'. Only .txt and .csv are allowed.\n";
        return false;
    }

    Directory* dir = currentDir();
    if (!dir) return false;

    if (dir->files.count(name)) {
        std::cout << "Error: file '" << name << "' already exists.\n";
        return false;
    }

    File file(name, content, password);
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

    const File& file = it->second;
    std::cout << "\n── " << file.name << " [" << fileTypeToString(file.type) << ", ." << file.extension << "] ──────────────────────────\n";

    if (file.type == FileType::Csv) displayCsvTable(file.content);
    else std::cout << file.content << "\n";

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
    it->second.locked = !newPassword.empty();
    std::cout << "Password updated for: " << name << "\n";
    return true;
}

bool FileSystem::removePassword(const std::string& name,
                                const std::string& password) {
    return setPassword(name, password, "");
}

bool FileSystem::makeDirectory(const std::string& name) {
    Directory* dir = currentDir();
    if (!dir) return false;

    if (!isValidNamePart(name) || name.find('.') != std::string::npos) {
        std::cout << "Error: invalid directory name. Directory names should not contain slashes or file extensions.\n";
        return false;
    }

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

void FileSystem::printHelp() const {
    std::cout << R"(
  MockFS Commands
  ────────────────────────────────────────────────────────
  ls                              List current directory
  cd <dir>                        Change directory (use .. to go up)
  mkdir <dir>                     Create a directory
  rmdir <dir>                     Remove an empty directory
  pwd                             Print working directory

  touch <file.txt|file.csv>       Create an empty .txt or .csv file
  write <file.txt>                Create a text file interactively
  csv <file.csv>                  Create a CSV file interactively
  cat <file.txt|file.csv> [pw]    Display text or CSV table
  edit <file.txt|file.csv> [pw]   Edit file content interactively
  rm <file.txt|file.csv> [pw]     Delete a file

  passwd <file.txt|file.csv>      Set or change a file's password
  unpasswd <file.txt|file.csv>    Remove password from a file

  Supported file extensions: .txt .csv only

  help                            Show this help
  exit                            Quit the shell
  ────────────────────────────────────────────────────────
)";
}

void runShell(FileSystem& fs, std::istream& in) {
    std::cout << "\n  MockFS  —  type 'help' for commands\n\n";

    std::string line;
    while (true) {
        std::cout << "mockfs:" << fs.currentPath() << "$ ";
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
            std::string pw;
            std::cout << "Password protect? (y/n): ";
            std::string answer;
            std::getline(in, answer);
            if (!answer.empty() && (answer[0] == 'y' || answer[0] == 'Y')) pw = readPassword("Set password: ", in);
            fs.createTextFile(args[1], content, pw);
        } else if (cmd == "csv") {
            if (args.size() < 2) { std::cout << "Usage: csv <file.csv>\n"; continue; }
            std::string content = readMultilineContent(args[1], in);
            std::string pw;
            std::cout << "Password protect? (y/n): ";
            std::string answer;
            std::getline(in, answer);
            if (!answer.empty() && (answer[0] == 'y' || answer[0] == 'Y')) pw = readPassword("Set password: ", in);
            fs.createCsvFile(args[1], content, pw);
        } else if (cmd == "cat") {
            if (args.size() < 2) { std::cout << "Usage: cat <file.txt|file.csv> [password]\n"; continue; }
            std::string pw = (args.size() >= 3) ? args[2] : "";
            if (pw.empty()) pw = readPassword("Password (leave blank if none): ", in);
            fs.displayFile(args[1], pw);
        } else if (cmd == "edit") {
            if (args.size() < 2) { std::cout << "Usage: edit <file.txt|file.csv>\n"; continue; }
            std::string pw = (args.size() >= 3) ? args[2] : readPassword("Password (leave blank if none): ", in);
            std::string content = readMultilineContent(args[1], in);
            fs.editFile(args[1], content, pw);
        } else if (cmd == "rm") {
            if (args.size() < 2) { std::cout << "Usage: rm <file.txt|file.csv>\n"; continue; }
            std::string pw = (args.size() >= 3) ? args[2] : readPassword("Password (leave blank if none): ", in);
            fs.deleteFile(args[1], pw);
        } else if (cmd == "passwd") {
            if (args.size() < 2) { std::cout << "Usage: passwd <file.txt|file.csv>\n"; continue; }
            std::string oldPw = readPassword("Current password (blank if none): ", in);
            std::string newPw = readPassword("New password: ", in);
            std::string confirmPw = readPassword("Confirm new password: ", in);
            if (newPw != confirmPw) { std::cout << "Error: passwords do not match.\n"; continue; }
            fs.setPassword(args[1], oldPw, newPw);
        } else if (cmd == "unpasswd") {
            if (args.size() < 2) { std::cout << "Usage: unpasswd <file.txt|file.csv>\n"; continue; }
            std::string pw = readPassword("Current password: ", in);
            fs.removePassword(args[1], pw);
        } else {
            std::cout << "Unknown command: '" << cmd << "'. Type 'help' for commands.\n";
        }
    }
}
