#pragma once
#include <switch.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
namespace ul::fs {
    inline std::string Map(const std::string &p){ return p; } // tests run in a cwd containing "sdmc:/"
    inline bool ExistsFile(const std::string &p){ struct stat st; return stat(Map(p).c_str(), &st) == 0 && S_ISREG(st.st_mode); }
    inline std::string JoinPath(const std::string &a, const std::string &b){ return a.empty() ? b : (a.back()=='/' ? a + b : a + "/" + b); }
    inline void CreateDirectory(const std::string &p){ mkdir(p.c_str(), 0777); }
}
