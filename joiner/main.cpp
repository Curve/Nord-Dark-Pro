#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <ostream>
#include <set>
#include <string>

auto generateColorMap(const nlohmann::json &nordPro, const nlohmann::json &oneDark)
{
    std::set<std::string> conflicts;
    std::map<std::string, std::string> rtn;

    for (const auto &nordItem : nordPro["tokenColors"])
    {
        auto nordName = nordItem.at("name").get<std::string>(); // Always present in nordTheme.
        auto nordColor = nordItem["settings"]["foreground"].get<std::string>();

        for (const auto &oneDarkItem : oneDark["tokenColors"])
        {
            if (oneDarkItem.count("settings"))
            {
                if (oneDarkItem["settings"].count("foreground"))
                {
                    auto oneDarkName = oneDarkItem.value("name", "");
                    auto oneDarkColor = oneDarkItem["settings"]["foreground"].get<std::string>();

                    if (nordName == oneDarkName)
                    {
                        if (!rtn.count(oneDarkColor))
                        {
                            rtn[oneDarkColor] = nordColor;
                        }
                        else
                        {
                            if (rtn[oneDarkColor] != nordColor)
                            {
                                std::cout << "'" << oneDarkName << "'(" << oneDarkColor
                                          << ") has different values, current: " << rtn[oneDarkColor]
                                          << ", new: " << nordColor << std::endl;
                                conflicts.emplace(oneDarkColor);
                            }
                        }
                    }
                }
            }
        }
    }

    for (const auto &conflict : conflicts)
    {
        if (!rtn.count(conflict))
            continue;

        std::cout << "'" << conflict << "'"
                  << " and '" << rtn[conflict] << "' are conflicting, replace conflict? [y/n]" << std::endl;

        char a = 'n';
        std::cin >> a;

        if (a == 'y')
        {
            std::cout << "Enter new color: ";

            std::string newColor;
            std::cin >> newColor;

            rtn[conflict] = newColor;
        }
    }

    return rtn;
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

    auto colorMap = generateColorMap(nordPro, oneDark);
    std::map<std::string, std::string> nordItems;
    for (const auto &item : nordPro["tokenColors"])
    {
        if (item.count("name"))
        {
            nordItems.emplace(item["name"], item["settings"]["foreground"]);
        }
    }

    std::set<std::string> oneDarkItems;
    for (auto &tokenItem : oneDark["tokenColors"])
    {
        if (tokenItem.count("name"))
        {
            oneDarkItems.emplace(tokenItem["name"]);
        }

        if (tokenItem.count("settings"))
        {
            if (tokenItem["settings"].count("foreground"))
            {
                auto color = tokenItem["settings"]["foreground"].get<std::string>();
                if (colorMap.count(color))
                {
                    auto name = tokenItem.value("name", "");
                    if (!name.empty() && nordItems.count(name))
                    {
                        std::cout << "Forcing Color for " << name << std::endl;
                        tokenItem["settings"]["foreground"] = nordItems.at(name);
                        continue;
                    }

                    tokenItem["settings"]["foreground"] = colorMap[color];
                }
            }
        }
    }

    for (const auto &tokenItem : nordPro["tokenColors"])
    {
        if (!oneDarkItems.count(tokenItem["name"]))
        {
            oneDark["tokenColors"].push_back(tokenItem);
            std::cout << "Adding missing " << tokenItem["name"] << std::endl;
        }
    }

    nordPro["tokenColors"] = oneDark["tokenColors"];

    std::ofstream output("output.json");
    output << nordPro.dump() << std::endl;
    output.close();

    return 0;
}