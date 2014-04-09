#include "util.h"
#include <fstream>
#include "lexer.h"

using namespace bc;
using namespace bc::lex;
using bc::lex::bcTokenType;

std::string util::bcitos(int in)
{
	std::string s;
	std::stringstream out;
	out << in;
	return out.str();
}
	

int util::bcstoi(std::string in)
{	
	int retval;
	std::stringstream convert(in);
	if ( !(convert >> retval) )
	{
		return 0;
	}
	else
	{
		return retval;
	}
}

std::string util::bcftos(float in)
{
	std::string s;
	std::stringstream out;
	out << in;
	return out.str();
}
	

float util::bcstof(std::string in)
{	
	float retval;
	std::stringstream convert(in);
	if ( !(convert >> retval) )
	{
		return 0;
	}
	else
	{
		return retval;
	}
}

std::string util::bctolower(std::string in)
{
	for (int i=0;i<strlen(in.c_str());i++)
	if (in[i] >= 0x41 && in[i] <= 0x5A)
	{
		in[i] = in[i] + 0x20;
	}
	return in;
}

std::string util::bctolower(char in)
{
	std::string out;
	if (in >= 0x41 && in <= 0x5A)
	{
		in = in + 0x20;
	}
	out+=in;
	return out;
}

bool util::bcfexists(const char *filename)
{
	std::ifstream ifile(filename);
	return ifile.good();
}

bool util::bcreadfile(const char* in,std::vector<std::string>* out)
{
	std::string line;
	std::ifstream myfile(in);
  
	if (myfile.is_open())
	{
		while ( myfile.good() )
		{
			getline(myfile,line);
			line.append("\n");
			out->push_back( line );		
		}
		myfile.close();
		return true;
	}
	else
	{
		return false;
	}
}

int util::GetPrecedence(lex::bcToken tokin)
{
	switch(tokin.type)
	{
	case tt_logor:
		return 14;

	case tt_logand:
		return 13;

	case tt_equal:
	case tt_notequal:
		return 12;

	case tt_greater:
	case tt_less:
	case tt_lessequal:
	case tt_greaterequal:
		return 10;

	case tt_mult:
	case tt_div:
		return 8;

	case tt_plus:
	case tt_minus:
		return 6;

	case tt_lognot:
		return 4;

	case tt_cparen:
		return -1;

	default:
		return 0;
	}
}

int util::IsOperator(lex::bcToken tokin)
{
	switch(tokin.type)
	{
	case tt_mult:
	case tt_div:
	case tt_plus:
	case tt_minus:
		return true;
	}
	return false;
}