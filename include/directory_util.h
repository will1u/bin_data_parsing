#include <cstdlib>
#include <string>

namespace DirectoryUtil {
    std::string expandTilde(const std::string& path) {
        if (!path.empty() && path[0] == '~') {
            const char* home = std::getenv("HOME");
            if (home) {
                return std::string(home) + path.substr(1);
            }
        }
        return path; // Return as is if no tilde
    }
}

