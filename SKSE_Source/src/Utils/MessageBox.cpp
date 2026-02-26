#include "MessageBox.h"
#include "RE/M/MessageBoxMenu.h"

namespace HotSwap::Utils {

    namespace {
        class MessageBoxCallback : public RE::IMessageBoxCallback {
        public:
            explicit MessageBoxCallback(std::function<void(uint32_t)> callback) : _callback{std::move(callback)} {}
            ~MessageBoxCallback() override = default;

            void Run(RE::IMessageBoxCallback::Message message) override {
                _callback(static_cast<uint32_t>(message));
            }

        private:
            std::function<void(uint32_t)> _callback;
        };
    }

    void ShowMessageBox(
        const std::string& body,
        const std::vector<std::string>& buttons,
        std::function<void(uint32_t)> callback)
    {
        SKSE::GetTaskInterface()->AddTask([body, buttons, callback = std::move(callback)]() mutable {
            auto* messagebox = RE::MessageDataFactoryManager::GetSingleton()
                ->GetCreator<RE::MessageBoxData>(RE::InterfaceStrings::GetSingleton()->messageBoxData)
                ->Create();
            messagebox->callback = RE::make_smart<MessageBoxCallback>(std::move(callback));
            messagebox->bodyText = body;
            for (const auto& text : buttons) {
                messagebox->buttonText.push_back(text.c_str());
            }
            RE::MessageBoxMenu::QueueMessage(messagebox);
        });
    }
}
