#include <cstdlib>
#include <algorithm>
#include <functional>
#include <numeric>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <cassert>

#if defined(_WIN32)
#define listcmd "dir /B *.dat > dataFiles.txt"
#define erasecmd "erase dataFiles.txt"
#elif defined(__GNUC__)
#define listcmd "ls -1 *.dat > dataFiles.txt"
#define erasecmd "rm dataFiles.txt"
#else
#error Unsupported compiler.
#endif

using namespace std;

int main(int argc, char** argv) {
	//Make sure user provided a config.ini file
	if (!argv[1]) {
		cout << "No parameters found." << endl;
		return 0;
	}
	else if (strcmp(argv[1], "config.ini") != 0) {
		cout << "Invalid configuration file." << endl;
		return 0;
	}

	//Verify all the necessary parameters are in the ini file
	double drop_ratio = -1;
	int vt = -1, width = -1, pulse_delta = -1, below_drop_ratio = -1;
	ifstream ifs(argv[1], ifstream::in);
	string parameter = "";
	vector<string> params;

	//I would've used a copy algortihm with a back_inserter here, but
	//there was no easy way to get a full linefor each iteration.
	//in short, using a while loop is more readable.
	while (getline(ifs, parameter)) params.push_back(parameter);
	for (const string& param : params) {
		//Scan for parameters and apply them
		if (param[0] == '#') continue;
		string var = param.substr(0, param.find('=')), value = "";
		if (var.find_first_not_of(" \n")) continue;
		else if (var == "vt") {
			value = param.substr(param.find('=') + 1);
			vt = stoi(value);
		}
		else if (var == "width") {
			value = param.substr(param.find('=') + 1);
			width = stoi(value);
		}
		else if (var == "pulse_delta") {
			value = param.substr(param.find('=') + 1);
			pulse_delta = stoi(value);
		}
		else if (var == "drop_ratio") {
			value = param.substr(param.find('=') + 1);
			drop_ratio = stod(value);
		}
		else if (var == "below_drop_ratio") {
			value = param.substr(param.find('=') + 1);
			below_drop_ratio = stoi(value);
		}
		else {
			cout << "Invalid Config.ini file." << endl;
			getchar();
			throw 0;
		}
	} //End of for-loop
	ifs.close();
	assert(vt != -1);
	assert(width != -1);
	assert(pulse_delta != -1);
	assert(drop_ratio != -1);
	assert(below_drop_ratio != -1);

	//Find all .dat files and push their names into vector
	system(listcmd);

	vector<string> files;
	string fileName = "";
	ifs.open("dataFiles.txt", ifstream::in);
	while (getline(ifs, fileName))	files.push_back(fileName);		//TODO: Find algorithm to replace this
	ifs.close();
	system(erasecmd);

	//go through one .dat file at a time. Analyze its data for pulses
	auto smooth = [](int& iter) {return (*(&iter - 3) + 2 * *(&iter - 2) + 3 * *(&iter - 1) + 3 * iter + 3 * *(&iter + 1) + 2 * *(&iter + 2) + *(&iter + 3)) / 15; };
	auto isPeak = [vt](int& i, int j) { return ((*(&i + 2) - i) > vt) ? true : false; };
	vector<int> data, smoothData, pulses, pulseArea;
	int area = 0;
	for (const string& file : files) {
		//open file, copy contents into vector, close file.
		ifs.open(file, ifstream::in);
		copy(istream_iterator<int>(ifs), istream_iterator<int>(), back_inserter(data));
		ifs.close();

		//Negate all data, then copy smoothened data into new vector
		transform(begin(data), end(data), begin(data), [](int x) {return -x; });
		copy(begin(data), begin(data) + 3, back_inserter(smoothData));
		transform(begin(data) + 3, end(data) - 2, back_inserter(smoothData), smooth);
		copy(end(data) - 2, end(data), back_inserter(smoothData));

		//Go through all data in .dat file loking for multiple pulses.
		auto it = begin(smoothData), pulseS = end(smoothData), nPulse = end(smoothData), pulseP = end(smoothData);
		while (it < end(smoothData) - 3) {
			//Look for very first pulse in smoothData
			if (pulseS == end(smoothData)) {
				it = adjacent_find(it, end(smoothData), isPeak);
				pulseS = it;
				if (it == end(smoothData)) break;
			}

			//find point where values start decreasing
			it = adjacent_find(it + 2, end(smoothData), greater<int>());
			pulseP = it;

			//Check if it piggybacks another pulse -- Find next pulse
			it = adjacent_find(it, end(smoothData) - 3, isPeak);

			if (it < end(smoothData) - 3) {
				nPulse = it;

				//is within pulse_delta?
				if (nPulse - pulseS <= pulse_delta) {
					int piggyCriteria = drop_ratio * 1.5 * *pulseP;
					int count = count_if(pulseP + 1, nPulse, [piggyCriteria](int x) { return x < piggyCriteria; });
					if (count > below_drop_ratio) pulseS = nPulse;
				}
			}
			else nPulse = end(smoothData);

			//measure area
			int pulseStart = (pulseS - begin(smoothData)), nextPulse = (nPulse - begin(smoothData));
			if (pulseS == nPulse) continue;
			else if (nPulse == end(smoothData)) area = accumulate(begin(data) + pulseStart, begin(data) + pulseStart + width, 0);
			else area = accumulate(begin(data) + pulseStart, begin(data) + nextPulse, 0);
			pulses.push_back(pulseStart);
			pulseArea.push_back(area);
			pulseS = nPulse;
		}

		//Display pulses and their areas
		if (pulses.size() > 0) {
			cout << file << "\nIndex:\t";
			copy(begin(pulses), end(pulses), ostream_iterator<int>(cout, "\t"));
			cout << "\nArea:\t";
			copy(begin(pulseArea), end(pulseArea), ostream_iterator<int>(cout, "\t"));
			cout << '\n' << endl;
		}

		pulses.clear();
		pulseArea.clear();
		data.clear();
		smoothData.clear();
	}

	cin.clear();
}