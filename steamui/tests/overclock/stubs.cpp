// Harness stand-ins for the hardware layer sys-clk talks to (Board / pm / logging / libnx sync).
#include "board.h"
#include "process_management.h"
#include "file_utils.h"
#include <cstdarg>
#include <cstdio>
#include <vector>
SysClkProfile g_Profile = SysClkProfile_Handheld; SysClkSocType g_Soc = SysClkSocType_Erista;
std::uint64_t g_Tid = PROCESS_MANAGEMENT_QLAUNCH_TID; bool g_Verbose = false;
std::uint32_t g_Hz[SysClkModule_EnumMax] = { 1020000000, 384000000, 1331200000 };
int g_SetCalls = 0, g_ResetCalls = 0;
static const std::vector<std::uint32_t> kCpu = { 204000000, 306000000, 408000000, 510000000, 612000000, 714000000, 816000000, 918000000, 1020000000, 1122000000, 1224000000, 1326000000, 1428000000, 1581000000, 1683000000, 1785000000 };
static const std::vector<std::uint32_t> kGpu = { 76800000, 153600000, 230400000, 307200000, 384000000, 460800000, 537600000, 614400000, 691200000, 768000000, 844800000, 921600000 };
static const std::vector<std::uint32_t> kMem = { 204000000, 665600000, 800000000, 1065600000, 1331200000, 1600000000 };
const char* Board::GetProfileName(SysClkProfile p, bool pretty){ return sysclkFormatProfile(p, pretty); }
const char* Board::GetModuleName(SysClkModule m, bool pretty){ return sysclkFormatModule(m, pretty); }
const char* Board::GetThermalSensorName(SysClkThermalSensor s, bool pretty){ return sysclkFormatThermalSensor(s, pretty); }
const char* Board::GetPowerSensorName(SysClkPowerSensor s, bool pretty){ return sysclkFormatPowerSensor(s, pretty); }
void Board::Initialize(){} void Board::Exit(){}
void Board::ResetToStock(){ g_ResetCalls++; g_Hz[0] = 1020000000; g_Hz[1] = g_Profile == SysClkProfile_Docked ? 768000000 : 384000000; g_Hz[2] = g_Profile == SysClkProfile_Docked ? 1600000000 : 1331200000; }
SysClkProfile Board::GetProfile(){ return g_Profile; }
void Board::SetHz(SysClkModule m, std::uint32_t hz){ g_SetCalls++; g_Hz[m] = hz; }
std::uint32_t Board::GetHz(SysClkModule m){ return g_Hz[m]; }
std::uint32_t Board::GetRealHz(SysClkModule m){ return g_Hz[m]; }
void Board::GetFreqList(SysClkModule m, std::uint32_t* out, std::uint32_t max, std::uint32_t* n){
    const auto &v = m == SysClkModule_CPU ? kCpu : m == SysClkModule_GPU ? kGpu : kMem; *n = 0; for(auto hz: v) if(*n < max) out[(*n)++] = hz; }
std::uint32_t Board::GetTemperatureMilli(SysClkThermalSensor){ return 40000; }
std::int32_t Board::GetPowerMw(SysClkPowerSensor){ return 0; }
std::uint32_t Board::GetRamLoad(SysClkRamLoad){ return 0; }
SysClkSocType Board::GetSocType(){ return g_Soc; }
void ProcessManagement::Initialize(){} void ProcessManagement::WaitForQLaunch(){} void ProcessManagement::Exit(){}
std::uint64_t ProcessManagement::GetCurrentApplicationId(){ return g_Tid; }
void FileUtils::LogLine(const char* f, ...){ if(!g_Verbose) return; va_list a; va_start(a, f); std::vprintf(f, a); va_end(a); std::printf("\n"); }
void FileUtils::WriteContextToCsv(const SysClkContext*){}
bool FileUtils::IsLogEnabled(){ return false; }
extern "C" {
void svcSleepThread(s64){}
}
extern "C" { void mutexLock(Mutex*){} void mutexUnlock(Mutex*){} }
