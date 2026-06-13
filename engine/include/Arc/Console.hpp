#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>
#include <mutex>
#include <thread>
#include <type_traits>

namespace Arc
{
    using CVarValue = std::variant<int, float, std::string>;
    using CommandCallback = std::function<void(const std::vector<std::string>& args)>;

    struct CVar
    {
        std::string name;
        std::string description;
        CVarValue value;
        CVarValue defaultValue;
        std::function<void(const CVarValue&)> onChange;
    };

    class Console final
    {
    public:
        static Console& instance();

        Console(const Console&) = delete;
        Console& operator=(const Console&) = delete;
        Console(Console&&) = delete;
        Console& operator=(Console&&) = delete;

        void registerCVar(const std::string& name, const std::string& description, const CVarValue& defaultValue, std::function<void(const CVarValue&)> onChange = nullptr);
        void registerCommand(const std::string& name, CommandCallback callback);

        bool setCVar(const std::string& name, const CVarValue& value);
        [[nodiscard]] CVarValue getCVar(const std::string& name, const CVarValue& fallback = "") const;

        template<typename T>
        [[nodiscard]] T getCVarValue(const std::string& name, T fallback = T{}) const
        {
            auto var = getCVar(name, fallback);
            if (auto pVal = std::get_if<T>(&var))
            {
                return *pVal;
            }
            if constexpr (std::is_same_v<T, float>)
            {
                if (auto pInt = std::get_if<int>(&var))
                {
                    return static_cast<float>(*pInt);
                }
            }
            if constexpr (std::is_same_v<T, int>)
            {
                if (auto pFloat = std::get_if<float>(&var))
                {
                    return static_cast<int>(*pFloat);
                }
            }
            return fallback;
        }

        void execute(const std::string& commandLine);

        void startStdinThread();
        void stopStdinThread();

    private:
        Console();
        ~Console();

        mutable std::mutex m_mutex;
        std::unordered_map<std::string, CVar> m_cvars;
        std::unordered_map<std::string, CommandCallback> m_commands;

        std::thread m_stdinThread;
        bool m_stdinThreadRunning = false;
    };
}
