#pragma once
#include <string>
#include <vector>
#include <set>


std::vector<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::set<std::string_view> MakeUniqueNonEmptyStrings(const StringContainer &strings)
{
    std::set<std::string_view> non_empty_strings;
    for (const std::string_view &str : strings)
    {
        if (!str.empty())
        {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

//std::vector<std::string> SplitIntoWords(const std::string &text);

/*template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer &strings)
{
    std::set<std::string> non_empty_strings;
    for (const std::string &str : strings)
    {
        if (!str.empty())
        {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}*/
