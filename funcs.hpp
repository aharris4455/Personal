#ifndef funcs_hpp
#define funcs_hpp
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <utility>
#include <cmath>
#include <map>
#include <tuple>
#include <cctype>
#include <chrono>

using namespace std;

enum sentanceType{
    GPGGA,
    GPGSV,
    UNKNOWN
};
sentanceType getSentType(const string &sentID);
vector<string> split(const char* buf, char delim);
pair<double, double> getCoords(const vector<string> &sentence);
void updateCoords(double lat, double longi);
double convertToDecimal(double degMin, char hemisphere);
map<int, tuple<int, int, int> > getSatInfo(const vector<string> &sentence);
string trimNonNumeric(const string &str);
void updateSatInfo(int PRN, int ELE, int AZI, int SNR);

#endif