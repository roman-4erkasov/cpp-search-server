#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view str)
{
    vector<string_view> result;
    str.remove_prefix(
        std::min(
            str.find_first_not_of(' '), 
            str.size()
        )
    );
    
    while (0<str.size()) {
        int64_t space = str.find(' ');
        result.push_back(
            str.substr(0,space)
        );
        str.remove_prefix(
            std::min(
                str.size(),
                str.find(' ')
            )
        );
        str.remove_prefix(
            std::min(
                str.find_first_not_of(' '), 
                str.size()
            )
        );
    }
    return result;
}

// vector<string_view> SplitIntoWords(
//     string_view text
// )
// {
//     vector<string_view> words;
//     string_view word;
//     for (const char c : text)
//     {
//         if (c == ' ')
//         {
//             if (!word.empty())
//             {
//                 words.push_back(word);
//                 word.clear();
//             }
//         }
//         else
//         {
//             word += c;
//         }
//     }
//     if (!word.empty())
//     {
//         words.push_back(word);
//     }
// 
//     return words;
// }


/*vector<string> SplitIntoWords(const string &text)
{
    vector<string> words;
    string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}*/
