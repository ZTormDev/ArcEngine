#include "Arc/Console.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace Arc
{
    namespace
    {
        std::vector<std::string> parseCommandLine(const std::string& line)
        {
            std::vector<std::string> tokens;
            std::string current;
            bool insideQuotes = false;

            for (std::size_t i = 0; i < line.length(); ++i)
            {
                char c = line[i];
                if (c == '"')
                {
                    insideQuotes = !insideQuotes;
                }
                else if (c == ' ' && !insideQuotes)
                {
                    if (!current.empty())
                    {
                        tokens.push_back(current);
                        current.clear();
                    }
                }
                else
                {
                    current += c;
                }
            }

            if (!current.empty())
            {
                tokens.push_back(current);
            }

            return tokens;
        }

        std::string cvarValueToString(const CVarValue& value)
        {
            if (auto pInt = std::get_if<int>(&value))
            {
                return std::to_string(*pInt);
            }
            if (auto pFloat = std::get_if<float>(&value))
            {
                return std::to_string(*pFloat);
            }
            if (auto pStr = std::get_if<std::string>(&value))
            {
                return *pStr;
            }
            return "";
        }
    }

    Console& Console::instance()
    {
        static Console inst;
        return inst;
    }

    Console::Console()
    {
        // Register default built-in commands
        registerCommand("help", [this](const std::vector<std::string>&) {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::cout << "\n--- Arc Engine Help ---\n";
            std::cout << "Available CVars:\n";
            for (const auto& [name, cvar] : m_cvars)
            {
                std::cout << "  " << name << " = " << cvarValueToString(cvar.value)
                          << " (default: " << cvarValueToString(cvar.defaultValue) << ") - "
                          << cvar.description << "\n";
            }
            std::cout << "Available Commands:\n";
            for (const auto& [name, _] : m_commands)
            {
                std::cout << "  " << name << "\n";
            }
            std::cout << "-----------------------\n" << std::endl;
        });

        registerCommand("list", [this](const std::vector<std::string>& args) {
            execute("help");
            (void)args;
        });
    }

    Console::~Console()
    {
        stopStdinThread();
    }

    void Console::registerCVar(const std::string& name, const std::string& description, const CVarValue& defaultValue, std::function<void(const CVarValue&)> onChange)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_cvars.find(name) != m_cvars.end())
        {
            return; // Already registered
        }

        CVar cvar{
            name,
            description,
            defaultValue,
            defaultValue,
            onChange
        };
        m_cvars[name] = cvar;
    }

    void Console::registerCommand(const std::string& name, CommandCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_commands[name] = callback;
    }

    bool Console::setCVar(const std::string& name, const CVarValue& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cvars.find(name);
        if (it == m_cvars.end())
        {
            return false;
        }

        CVar& cvar = it->second;

        // Ensure types match or can be converted
        CVarValue convertedValue = value;
        if (cvar.value.index() != value.index())
        {
            // Type mismatch, try conversion
            if (std::holds_alternative<int>(cvar.value) && std::holds_alternative<float>(value))
            {
                convertedValue = static_cast<int>(std::get<float>(value));
            }
            else if (std::holds_alternative<float>(cvar.value) && std::holds_alternative<int>(value))
            {
                convertedValue = static_cast<float>(std::get<int>(value));
            }
            else if (std::holds_alternative<std::string>(cvar.value))
            {
                convertedValue = cvarValueToString(value);
            }
            else
            {
                return false; // Incompatible types
            }
        }

        cvar.value = convertedValue;
        if (cvar.onChange != nullptr)
        {
            cvar.onChange(cvar.value);
        }
        return true;
    }

    CVarValue Console::getCVar(const std::string& name, const CVarValue& fallback) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cvars.find(name);
        if (it == m_cvars.end())
        {
            return fallback;
        }
        return it->second.value;
    }

    void Console::execute(const std::string& commandLine)
    {
        std::vector<std::string> tokens = parseCommandLine(commandLine);
        if (tokens.empty())
        {
            return;
        }

        std::string name = tokens[0];
        // Lowercase names for case insensitivity
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        std::vector<std::string> args(tokens.begin() + 1, tokens.end());

        // Check if it's a command
        CommandCallback cmdCallback = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_commands.find(name);
            if (it != m_commands.end())
            {
                cmdCallback = it->second;
            }
        }

        if (cmdCallback != nullptr)
        {
            cmdCallback(args);
            return;
        }

        // Check if it's a CVar
        bool isCVar = false;
        CVar cvarCopy;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_cvars.find(name);
            if (it != m_cvars.end())
            {
                isCVar = true;
                cvarCopy = it->second;
            }
        }

        if (isCVar)
        {
            if (args.empty())
            {
                std::cout << cvarCopy.name << " = \"" << cvarValueToString(cvarCopy.value) << "\""
                          << " (default: \"" << cvarValueToString(cvarCopy.defaultValue) << "\") - "
                          << cvarCopy.description << std::endl;
            }
            else
            {
                std::string argStr = args[0];
                CVarValue newVal;

                if (std::holds_alternative<int>(cvarCopy.value))
                {
                    try
                    {
                        newVal = std::stoi(argStr);
                    }
                    catch (const std::exception&)
                    {
                        std::cerr << "Error: CVar '" << cvarCopy.name << "' requires an integer value." << std::endl;
                        return;
                    }
                }
                else if (std::holds_alternative<float>(cvarCopy.value))
                {
                    try
                    {
                        newVal = std::stof(argStr);
                    }
                    catch (const std::exception&)
                    {
                        std::cerr << "Error: CVar '" << cvarCopy.name << "' requires a float value." << std::endl;
                        return;
                    }
                }
                else
                {
                    newVal = argStr;
                }

                if (setCVar(cvarCopy.name, newVal))
                {
                    std::cout << cvarCopy.name << " set to \"" << cvarValueToString(newVal) << "\"" << std::endl;
                }
                else
                {
                    std::cerr << "Error setting CVar '" << cvarCopy.name << "'" << std::endl;
                }
            }
            return;
        }

        std::cerr << "Unknown command or CVar: '" << name << "'. Type 'help' for a list of commands." << std::endl;
    }

    void Console::startStdinThread()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stdinThreadRunning)
        {
            return;
        }

        m_stdinThreadRunning = true;
        m_stdinThread = std::thread([this]() {
            std::string line;
            while (m_stdinThreadRunning)
            {
                std::cout << "> " << std::flush;
                if (!std::getline(std::cin, line))
                {
                    break;
                }
                if (!m_stdinThreadRunning)
                {
                    break;
                }
                execute(line);
            }
        });
        m_stdinThread.detach();
    }

    void Console::stopStdinThread()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stdinThreadRunning = false;
    }
}
