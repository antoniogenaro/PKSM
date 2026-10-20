#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace pksm::switchui
{
    inline std::vector<std::string> filterMainSaveFiles(std::vector<std::string> files)
    {
        std::unordered_set<std::string> directoriesWithMain;
        for (const auto& file : files)
        {
            const auto separator = file.find_last_of('/');
            const std::string_view name =
                separator == std::string::npos ? file :
                                                  std::string_view(file).substr(separator + 1);
            if (name == "main")
            {
                directoriesWithMain.emplace(
                    separator == std::string::npos ? "" : file.substr(0, separator));
            }
        }

        std::erase_if(files, [&directoriesWithMain](const std::string& file)
        {
            const auto separator = file.find_last_of('/');
            const std::string_view name =
                separator == std::string::npos ? file :
                                                  std::string_view(file).substr(separator + 1);
            const std::string_view directory =
                separator == std::string::npos ? "" : std::string_view(file).substr(0, separator);
            return name != "main" && directoriesWithMain.contains(std::string(directory));
        });
        return files;
    }
}
