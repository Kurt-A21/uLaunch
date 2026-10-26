
#pragma once
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <string>

// steamui: turning uLaunch on/off without touching anything else in the HOME menu's contents folder.
//
// uLaunch's only file there is exefs.nsp (a copy is kept in ulaunch/bin/uSystem). NXThemes Installer puts the
// HOME menu theme in the same folder (romfs/), so deleting the whole folder - what uManager used to do - also
// erased your NXTheme. These helpers only add/remove exefs.nsp.
//   uLaunch active   -> steamui is the HOME menu
//   uLaunch inactive -> Nintendo's HOME menu, with your NXTheme
// All paths are relative to a root ("sdmc:" on the console) so this can be tested on a PC.
namespace ul::homemenu {

    constexpr const char ContentsDir[] = "/atmosphere/contents/0100000000001000";
    constexpr const char ActiveExefs[] = "/atmosphere/contents/0100000000001000/exefs.nsp";
    constexpr const char BaseExefs[] = "/ulaunch/bin/uSystem/exefs.nsp";
    constexpr const char DefaultRoot[] = "sdmc:";

    inline bool IsFile(const std::string &path) {
        struct stat st;
        return (stat(path.c_str(), &st) == 0) && S_ISREG(st.st_mode);
    }

    inline bool IsActive(const std::string &root = DefaultRoot) {
        return IsFile(root + ActiveExefs);
    }

    // Whether the backup copy exists (needed to switch back on later)
    inline bool HasBackup(const std::string &root = DefaultRoot) {
        return IsFile(root + BaseExefs);
    }

    inline bool CopyFile(const std::string &src, const std::string &dst) {
        auto in = fopen(src.c_str(), "rb");
        if(in == nullptr) {
            return false;
        }
        const auto tmp = dst + ".tmp";
        auto out = fopen(tmp.c_str(), "wb");
        if(out == nullptr) {
            fclose(in);
            return false;
        }
        static char buf[0x10000];
        auto ok = true;
        size_t n;
        while((n = fread(buf, 1, sizeof(buf), in)) > 0) {
            if(fwrite(buf, 1, n, out) != n) {
                ok = false;
                break;
            }
        }
        ok = ok && !ferror(in);
        fclose(in);
        ok = (fclose(out) == 0) && ok;
        if(!ok) {
            remove(tmp.c_str());
            return false;
        }
        remove(dst.c_str());
        return rename(tmp.c_str(), dst.c_str()) == 0;
    }

    // Switches to Nintendo's HOME menu. Refuses (returns false) if the backup copy is missing, so you can always come back.
    inline bool Deactivate(const std::string &root = DefaultRoot) {
        if(!HasBackup(root)) {
            return false;
        }
        if(IsActive(root)) {
            remove((root + ActiveExefs).c_str());
        }
        // Leave no empty folder behind (stock state), but never remove one that still holds a theme
        rmdir((root + ContentsDir).c_str());
        return !IsActive(root);
    }

    // Switches back to uLaunch/steamui
    inline bool Activate(const std::string &root = DefaultRoot) {
        if(!HasBackup(root)) {
            return false;
        }
        mkdir((root + "/atmosphere").c_str(), 0777);
        mkdir((root + "/atmosphere/contents").c_str(), 0777);
        mkdir((root + ContentsDir).c_str(), 0777);
        return CopyFile(root + BaseExefs, root + ActiveExefs);
    }

}
