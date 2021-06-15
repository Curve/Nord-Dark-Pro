#include "vivid/interpolation.h"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <ostream>
#include <set>
#include <string>
#include <vivid/vivid.h>

const std::map<std::string, std::string> nordColors = {
    // Polar Night
    {"nord0", "#2e3440"}, //-> Used for background colors
    {"nord1", "#3b4252"}, //-> Elevated more prominent UI elements like "stauts bars", "panels", "modals", "buttons"
    {"nord2", "#434c5e"}, //-> Currently Active editor line, text selection / highlight
    {"nord3", "#4c566a"}, //-> Indent wrap / guide marker, comments, invisible / non-printable characters

    // Snow Storm
    {"nord4", "#d8dee9"}, //-> Text Editor Caret, color for variables, constants, attributes and fields
    {"nord5", "#e5e9f0"}, //-> subtle UI Text elements, button hover / focus
    {"nord6", "#eceff4"}, //-> Elevated UI text elements, plain text, curly / square brackets

    // Frost
    {"nord7", "#8fbcbb"},  //-> UI elements that stand out / require attention, classes, types & primitives
    {"nord8", "#88c0d0"},  //-> UI elements that require most attention, used for declarations, calls / functions.
    {"nord9", "#81a1c1"},  //-> Secondary UI elements, operators, units, punctuation
    {"nord10", "#5e81ac"}, //-> pragmas, comment keywords, preprocessor statements
    //
    {"nord11", "#bf616a"}, //-> Errors
    {"nord12", "#d08770"}, //-> Annotations and decorators
    {"nord13", "#ebcb8b"}, //-> Warnings
    {"nord14", "#a3be8c"}, //-> Success, strings of any type
    {"nord15", "#b48ead"}, //-> Numbers

};

auto getScopes(const nlohmann::json &item)
{
    std::vector<std::string> scopes;

    if (item.count("scope"))
    {
        if (item["scope"].is_array())
        {
            for (const auto &scope : item["scope"])
            {
                scopes.emplace_back(scope.get<std::string>());
            }
        }
        else if (item["scope"].is_string())
        {
            auto scope = item["scope"].get<std::string>();

            if (scope.find(',') != std::string::npos)
            {
                while (scope.find(',') != std::string::npos)
                {
                    auto splitted = scope.substr(0, scope.find_first_of(','));
                    scopes.emplace_back(splitted);

                    scope = scope.substr(scope.find_first_of(',') + 1);
                }
            }
            else
            {
                scopes.emplace_back(scope);
            }
        }
    }

    return scopes;
}

auto similarScopes(const nlohmann::json &nordItem, const nlohmann::json &oneDarkItem)
{
    auto nordScopes = getScopes(nordItem);
    auto oneDarkScopes = getScopes(oneDarkItem);

    if (nordScopes.empty() || oneDarkScopes.empty())
    {
        return false;
    }

    for (const auto &scope : nordScopes)
    {
        if (std::find(oneDarkScopes.begin(), oneDarkScopes.end(), scope) != oneDarkScopes.end())
        {
            return true;
        }
    }

    return false;
}

auto translateColor(const std::string &name)
{
    if (name.size() < 4)
    {
        return name;
    }

    if (name.substr(0, 4) == "nord")
    {
        auto colorName = name;
        if (name.find(' ') != std::string::npos)
        {
            colorName = name.substr(0, name.find_first_of(' '));
        }

        auto color = nordColors.at(colorName);
        if (name.find(' ') != std::string::npos)
        {
            auto vividColor = vivid::Color(color);
            auto params = name.substr(name.find_first_of(' ') + 1);

            if (!params.empty())
            {
                if (params[0] == 'd')
                {
                    auto amount = std::stof(params.substr(1));
                    auto hsl = vividColor.hsl().value();

                    auto l = hsl.z;
                    l -= amount / 100.f;
                    l = static_cast<float>(std::fmin(1, fmax(0, l)));

                    auto newColor = vivid::Color({hsl.x, hsl.y, l}, vivid::Color::Space::Hsl);
                    color = newColor.hex();
                }
                if (params[0] == 'l')
                {
                    auto amount = std::stof(params.substr(1));
                    auto hsl = vividColor.hsl().value();

                    auto l = hsl.z;
                    l += amount / 100.f;
                    l = static_cast<float>(std::fmin(1, fmax(0, l)));

                    auto newColor = vivid::Color({hsl.x, hsl.y, l}, vivid::Color::Space::Hsl);
                    color = newColor.hex();
                }
            }
        }

        return color;
    }

    return name;
}

auto generateColorMap(const nlohmann::json &nordPro, const nlohmann::json &oneDark)
{
    std::set<std::string> conflicts;
    std::map<std::string, std::string> colorMap;

    for (const auto &nordItem : nordPro["tokenColors"])
    {
        for (const auto &oneDarkItem : oneDark["tokenColors"])
        {
            if (similarScopes(nordItem, oneDarkItem))
            {
                if (oneDarkItem.count("settings"))
                {
                    if (oneDarkItem["settings"].count("foreground"))
                    {
                        auto nordColor = nordItem["settings"]["foreground"];
                        auto oneDarkColor = oneDarkItem["settings"]["foreground"];

                        if (!colorMap.count(oneDarkColor))
                        {
                            colorMap.emplace(oneDarkColor, nordColor);
                        }
                        else if (nordColor != colorMap[oneDarkColor])
                        {
                            conflicts.emplace(oneDarkColor);
                        }
                    }
                }
            }
        }
    }

    for (const auto &conflict : conflicts)
    {
        std::cout << "The colors " << conflict << " and " << colorMap[conflict] << " are in conflict, replace? [y/n]"
                  << std::endl;

        char choice = 'n';
        std::cin >> choice;

        if (choice == 'y')
        {
            std::cout << "Enter new color: ";

            std::string replacement;
            std::cin >> replacement;

            colorMap[conflict] = translateColor(replacement);
        }
    }

    return colorMap;
}

int main()
{
    nlohmann::json nordPro;
    {
        std::ifstream nordProFile("../../themes/Nord-Dark-Pro Base.json");
        std::string nordProContent((std::istreambuf_iterator<char>(nordProFile)), std::istreambuf_iterator<char>());
        nordProFile.close();
        nordPro = nlohmann::json::parse(nordProContent);
    }

    std::cout << "Do you wish to merge with one dark? [y/N]";
    char choice = 'n';
    std::cin >> choice;

    // Replace Color Names by corresponding color code
    for (auto &nordItem : nordPro["tokenColors"])
    {
        auto &color = nordItem["settings"]["foreground"];
        color = translateColor(color);
    }
    for (auto &nordItem : nordPro["colors"].items())
    {
        auto &color = nordItem.value();
        color = translateColor(color);
    }

    if (choice == 'y')
    {
        nlohmann::json oneDark;
        {
            std::ifstream oneDarkFile("../OneDark-Pro/themes/OneDark-Pro.json");
            std::string oneDarkContent((std::istreambuf_iterator<char>(oneDarkFile)), std::istreambuf_iterator<char>());
            oneDarkFile.close();
            oneDark = nlohmann::json::parse(oneDarkContent);
        }

        // Adjust Colors
        auto colorMap = generateColorMap(nordPro, oneDark);
        for (const auto &nordItem : nordPro["tokenColors"])
        {
            auto nordColor = nordItem["settings"]["foreground"];

            for (auto &oneDarkItem : oneDark["tokenColors"])
            {
                if (similarScopes(nordItem, oneDarkItem))
                {
                    std::cout << "Forcing '" << nordItem["name"] << "'" << std::endl;
                    oneDarkItem["settings"] = nordItem["settings"];
                    continue;
                }

                if (oneDarkItem.count("settings"))
                {
                    if (oneDarkItem["settings"].count("foreground"))
                    {
                        auto &oneDarkColor = oneDarkItem["settings"]["foreground"];

                        if (colorMap.count(oneDarkColor))
                        {
                            oneDarkColor = colorMap[oneDarkColor];
                        }
                    }
                }
            }
        }

        // Add Missing Items
        for (const auto &nordItem : nordPro["tokenColors"])
        {
            bool found = false;
            for (const auto &oneDarkItem : oneDark["tokenColors"])
            {
                if (similarScopes(nordItem, oneDarkItem))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::cout << "Adding '" << nordItem["name"] << "' to one dark" << std::endl;
                oneDark["tokenColors"].push_back(nordItem);
            }
        }

        nordPro["tokenColors"] = oneDark["tokenColors"];
    }

    std::ofstream output("output.json");
    output << nordPro.dump() << std::endl;
    output.close();

    return 0;
}