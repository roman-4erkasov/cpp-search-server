#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include "search_server.h"

void TestExcludeStopWordsFromAddedDocumentContent();
void TestExcludeMinusWordsFromAddedDocument();
int ComputeAverageRating(const std::vector<int> &ratings);
void TestRatingComputation();
void TestRelevanceOrder();
void TestPredicatFiltering();
void TestStatusFiltering();
std::vector<std::string> SplitText(const std::string &text);
double ComputeWordInverseDocumentFreq(
    const std::string &word, 
    int doc_count,
    std::map<std::string, std::map<int, double>> word_to_document_freqs_
);
void TestRelevanceComputation();
void TestAll();
