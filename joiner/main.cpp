#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <ostream>
#include <set>
#include <string>

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

            colorMap[conflict] = replacement;
        }
    }

    return colorMap;
}

int main()
{
    nlohmann::json nordPro;
    {
        std::ifstream nordProFile("../../themes/Nord-Dark-Pro.json");
        std::string nordProContent((std::istreambuf_iterator<char>(nordProFile)), std::istreambuf_iterator<char>());
        nordProFile.close();
        nordPro = nlohmann::json::parse(nordProContent);
    }

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
    std::ofstream output("output.json");
    output << nordPro.dump() << std::endl;
    output.close();

    return 0;
}