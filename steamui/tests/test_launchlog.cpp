#include <ul/menu/menu_LaunchLog.hpp>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <sys/stat.h>
int main(){ mkdir("sdmc:",0777); mkdir("sdmc:/ulaunch",0777); int fails=0;
  ul::menu::RecordLaunch("0100152000022000"); ul::menu::RecordLaunch("nfscx-vulkan"); ul::menu::RecordLaunch("");
  std::ifstream f("sdmc:/ulaunch/steamui/launches.txt"); std::string l; int n=0; std::regex re("^\\d{14} [0-9A-Za-z_.-]+$");
  while(std::getline(f,l)){ n++; if(!std::regex_match(l,re)){ std::printf("bad line: %s\n", l.c_str()); fails++; } }
  if(n!=2){ std::printf("expected 2 lines, got %d\n", n); fails++; }
  for(int i=0;i<2000;i++) ul::menu::RecordLaunch("0100152000022000");
  struct stat a, b; stat("sdmc:/ulaunch/steamui/launches.txt",&a); bool old = stat("sdmc:/ulaunch/steamui/launches.old.txt",&b)==0;
  if(a.st_size > 33*1024 || !old){ std::printf("rotation failed (%ld bytes, old=%d)\n",(long)a.st_size,old); fails++; }
  std::printf("%s: format ok, empty key ignored, file rotates at 32 KB (now %ld bytes)\n", fails?"FAILED":"PASSED", (long)a.st_size); return fails; }
