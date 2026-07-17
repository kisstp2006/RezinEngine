#pragma once

#include <string>
#include <vector>

using namespace std;
namespace Log
{

extern vector<string> logs;

void Info(const string& message);
void Warning(const string& message);
void Error(const string& message);

}
