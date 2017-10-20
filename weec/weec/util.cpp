#include "util.h"
#include <fstream>
#include <sstream>

using namespace std;
using namespace wc::util;

bool wc::util::fileExists(string p_filename)
{
	std::ifstream ifile(p_filename.c_str());
	return (bool)ifile;
}

vector<string> wc::util::loadFileAsText(string p_filename)
{
	vector<string> output;

	//chcek file exists
	if (!fileExists(p_filename))
		return output;

	//load each line as a string
	string lineBuffer;
	ifstream myfile(p_filename);
	while (getline(myfile, lineBuffer))
		output.push_back(lineBuffer);

	//return output
	return output;
}

int wc::util::randomInt()
{
	int min = INT_MIN;
	int max = INT_MAX;
	return min + (rand() % static_cast<int>(max - min + 1));
}

string wc::util::itos(int p_in)
{
	stringstream ss;
	ss << p_in;
	return ss.str();
}