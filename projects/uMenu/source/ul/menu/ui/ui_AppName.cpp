#include <ul/menu/ui/ui_AppName.hpp>
#include <ul/menu/ui/ui_Common.hpp>
#include <ul/ul_Result.hpp>
#include <memory>

namespace ul::menu::ui {

    std::string LoadApplicationName(const u64 app_id) {
        // Control data is large (~150 KB): allocate it instead of using the stack
        auto control = std::make_unique<NsApplicationControlData>();
        u64 size = 0;
        if(R_FAILED(nsextGetApplicationControlData(NsApplicationControlSource_Storage, app_id, control.get(), sizeof(NsApplicationControlData), &size))) {
            return "";
        }
        NacpLanguageEntry *lang_entry = nullptr;
        if(R_FAILED(nacpGetLanguageEntry(&control->nacp, &lang_entry)) || (lang_entry == nullptr)) {
            return "";
        }
        return lang_entry->name;
    }

}
