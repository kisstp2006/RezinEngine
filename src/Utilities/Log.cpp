#include <Rezin/Utilities/Log.hpp>

#include <iostream>

using namespace std;

namespace Log
{

vector<string> logs;

void Info(const string& message)
{
    cout << "INFO: " << message << endl;
}

void Warning(const string& message)
{
    cout << "WARNING: " << message << endl;
}

void Error(const string& message)
{
    cout << "ERROR: " << message << endl;
}

}
