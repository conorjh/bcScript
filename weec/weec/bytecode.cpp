#include "bytecode.h"
#include "vm.h"
#include "util.h"

using namespace std;
using namespace wc::util;
using namespace wc::vm;
using namespace wc::bytecode;
using namespace wc::util;

wc::bytecode::wcExecContext::wcExecContext()
{
	contextID = registers.pc = 0;
	registers.halt = false;
	targetPlatform = wcTargetPlatform::ct_bytecode;
}

wc::bytecode::wcInstruction::~wcInstruction()
{
}

wc::bytecode::wcInstruction::wcInstruction()
{
}

wc::bytecode::wcInstruction::wcInstruction(unsigned short int p_opcode)
{
	opcode = (wcOpcode)p_opcode;
}

wc::bytecode::wcInstructionPlusOperand::~wcInstructionPlusOperand()
{
}

wc::bytecode::wcInstructionPlusOperand::wcInstructionPlusOperand()
{

}

wc::bytecode::wcInstructionPlusOperand::wcInstructionPlusOperand(unsigned short int p_opcode, int p_op1)
{
	opcode = (wcOpcode)p_opcode;
	operand1 = p_op1;
}

wc::bytecode::wcInstructionPlusOperands::~wcInstructionPlusOperands()
{
}

wc::bytecode::wcInstructionPlusOperands::wcInstructionPlusOperands()
{
}

wc::bytecode::wcInstructionPlusOperands::wcInstructionPlusOperands(unsigned short int p_opcode, int p_op1, int p_op2)
{
	opcode = (wcOpcode)p_opcode;
	operand1 = p_op1;
	operand2 = p_op2;
}

void wc::bytecode::wcExecStack::push(wcChunk p_value)
{
	container.push_back(make_shared<wcChunk>(p_value));
}

void wc::bytecode::wcExecStack::push(wcChunki p_value)
{
	container.push_back(make_shared<wcChunki>(p_value));
}

void wc::bytecode::wcExecStack::push(wcChunkf p_value)
{
	container.push_back(make_shared<wcChunkf>(p_value));
}

void wc::bytecode::wcExecStack::push(wcChunks p_value)
{
	container.push_back(make_shared<wcChunks>(p_value));
}

shared_ptr<wcChunk> wc::bytecode::wcExecStack::pop()
{
	auto returnValue = container[container.size() - 1];
	container.erase(container.end() - 1);
	return returnValue;
}

std::shared_ptr<wcChunki> wc::bytecode::wcExecStack::popi()
{
	return static_pointer_cast<wcChunki>(pop());
}

std::shared_ptr<wcChunkf> wc::bytecode::wcExecStack::popf()
{
	return static_pointer_cast<wcChunkf>(pop());
}

std::shared_ptr<wcChunks> wc::bytecode::wcExecStack::pops()
{
	return static_pointer_cast<wcChunks>(pop());
}

shared_ptr<wcChunk> wc::bytecode::wcExecStack::peek(int p_index)
{
	return container[p_index];
}

std::shared_ptr<wcChunki> wc::bytecode::wcExecStack::peeki(int p_index)
{
	return static_pointer_cast<wcChunki>(peek(p_index));
}

std::shared_ptr<wcChunkf> wc::bytecode::wcExecStack::peekf(int p_index)
{
	return static_pointer_cast<wcChunkf>(peek(p_index));
}

std::shared_ptr<wcChunks> wc::bytecode::wcExecStack::peeks(int p_index)
{
	return static_pointer_cast<wcChunks>(peek(p_index));
}

shared_ptr<wcChunk> wc::bytecode::wcExecStack::top()
{
	if (!container.size())
		return shared_ptr<wcChunk>();
	return container[container.size()-1];
}

std::shared_ptr<wcChunki> wc::bytecode::wcExecStack::topi()
{
	return static_pointer_cast<wcChunki>(top());
}

std::shared_ptr<wcChunkf> wc::bytecode::wcExecStack::topf()
{
	return static_pointer_cast<wcChunkf>(top());
}

std::shared_ptr<wcChunks> wc::bytecode::wcExecStack::tops()
{
	return static_pointer_cast<wcChunks>(top());
}

void wc::bytecode::wcExecStack::set(int index, wcChunk value)
{
	*peek(index) = value;
}

void wc::bytecode::wcExecStack::set(int index, wcChunki value)
{
	*peeki(index) = value;
}

void wc::bytecode::wcExecStack::set(int index, wcChunkf value)
{
	*peekf(index) = value;
}

void wc::bytecode::wcExecStack::set(int index, wcChunks value)
{
	*peeks(index) = value;
}

int wc::bytecode::wcExecStack::size()
{
	return container.size();
}

void wc::bytecode::wcExecStack::clear()
{
	container.clear();
}

std::string wc::bytecode::wcStringTable::getString(unsigned int p_index)
{
	if (intTable.find(p_index) == intTable.end())
		return "";
	return intTable.find(p_index)->second;
}

unsigned int wc::bytecode::wcStringTable::getIndex(std::string p_str)
{
	if (strTable.find(p_str) == strTable.end())
		return 0;
	return strTable.find(p_str)->second;
}

bool wc::bytecode::wcStringTable::doesIndexExist(unsigned int p_index)
{
	if (intTable.find(p_index) == intTable.end())
		return false;
	return true;
}

bool wc::bytecode::wcStringTable::doesStringExist(std::string p_str)
{
	if (strTable.find(p_str) == strTable.end())
		return false;
	return true;
}

unsigned int wc::bytecode::wcStringTable::addEntry(std::string p_str)
{
	unsigned int newIndex = randomUInt();
	while (doesIndexExist(newIndex))
		newIndex = randomUInt();

	intTable.insert(std::make_pair(newIndex, p_str));
	strTable.insert(std::make_pair(p_str, newIndex));

	return newIndex;
}

wc::bytecode::wcChunk::wcChunk()
{

}

int wc::bytecode::wcChunk::i()
{
	return 0;
}

float wc::bytecode::wcChunk::f()
{
	return 0.0f;
}

std::string wc::bytecode::wcChunk::s()
{
	return std::string();
}

wc::bytecode::wcChunki::wcChunki(int p_i)
{
	_i = p_i;
}

int wc::bytecode::wcChunki::i()
{
	return _i;
}

float wc::bytecode::wcChunki::f()
{
	if (_i >= 0)
		return int(_i + 0.5);
	return int(_i - 0.5);
}

std::string wc::bytecode::wcChunki::s()
{
	return itos(_i);
}

wc::bytecode::wcChunkf::wcChunkf(float p_f)
{
	_f = p_f;
}

int wc::bytecode::wcChunkf::i()
{
	return _f;
}

float wc::bytecode::wcChunkf::f()
{
	return _f;
}

std::string wc::bytecode::wcChunkf::s()
{
	return util::ftos(_f);
}

wc::bytecode::wcChunks::wcChunks(string p_s)
{
	_s = p_s;
}

int wc::bytecode::wcChunks::i()
{
	return stoi(_s);
}

float wc::bytecode::wcChunks::f()
{
	return stof(_s);
}

std::string wc::bytecode::wcChunks::s()
{
	return _s;
}

wc::bytecode::wcExecContextRegisters::wcExecContextRegisters()
{
}

int& wc::bytecode::wcExecContextRegisters::operator[](int p_int)
{
	switch (p_int)
	{
	case wcRegisterTitles::pc:
		return pc;
	case wcRegisterTitles::t1:
		return t1;
	case wcRegisterTitles::t2:
		return t2;
	case wcRegisterTitles::cmp:
		return cmp;
	case wcRegisterTitles::eax:
		return eax;
	case wcRegisterTitles::instr:
		return instr;
	default:
	case wcRegisterTitles::halt:
		return halt;
	}
}
