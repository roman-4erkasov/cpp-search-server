#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
// #include "testing.h"
#include "search_server.h"

void TestExcludeStopWordsFromAddedDocumentContent();
void TestExcludeMinusWordsFromAddedDocument();
int ComputeAverageRating(const vector<int> &ratings);
void TestRatingComputation();
void TestRelevanceOrder();
void TestPredicatFiltering();
void TestStatusFiltering();
vector<string> SplitText(const string &text);
double ComputeWordInverseDocumentFreq(
    const string &word, 
    int doc_count,
    map<string, map<int, double>> word_to_document_freqs_
);
void TestRelevanceComputation();
void TestAll();

