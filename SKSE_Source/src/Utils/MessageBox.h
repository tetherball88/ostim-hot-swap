#pragma once
#include <functional>
#include <vector>
#include <string>

namespace HotSwap::Utils {
    // Show a Skyrim message box dispatched via SKSE task interface.
    // callback receives 0-based index of the selected button.
    void ShowMessageBox(
        const std::string& body,
        const std::vector<std::string>& buttons,
        std::function<void(uint32_t)> callback
    );
}
