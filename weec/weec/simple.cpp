#include "simple.h"
#include "codegen.h"
#include "parse.h"
#include "util.h"
#include <vector>

using namespace std;
using namespace wc;
using namespace wc::bytecode;
using namespace wc::parse;
using namespace wc::api;
using namespace wc::lex;
using namespace wc::codegen;
using namespace wc::util;
using namespace wc::vm;

namespace wc
{
	namespace codegen
	{
		inline vector<wcParseNode> genSimpExpression_genRPN(wcParseIndex& p_index, bytecode::wcSimpleExecContext& output, int startingDepth, vector<wcParseNode>* rpnOutput);
		inline vector<shared_ptr<wcInstruction>> genSimpExpression_genInstructionsFromRPN(wcParseIndex& p_index, bytecode::wcSimpleExecContext& output, vector<wcParseNode>* rpnOutput);
	}
}

vector<shared_ptr<wcInstruction>> wc::codegen::genSimpExpression(wcParseIndex& p_index, bytecode::wcSimpleExecContext& output, vector<wcParseNode>* rpnOutput = nullptr)
{
	if (p_index.getNode()->type != pn_exp)
		return vector<shared_ptr<wcInstruction>>();
	if (rpnOutput == nullptr)
		rpnOutput = &vector<wcParseNode>();	

	int startingDepth = p_index.getNodeDepth(p_index.getNode());
	p_index.nextNode();

	//output rpn to rpnOutput
	genSimpExpression_genRPN(p_index, output, startingDepth, rpnOutput);

	//emit simpcode based on the RPN
	vector<shared_ptr<wcInstruction>> instructionOutput = genSimpExpression_genInstructionsFromRPN(p_index, output, rpnOutput);

	return instructionOutput;
}

vector<wcParseNode> wc::codegen::genSimpExpression_genRPN(wcParseIndex& p_index, bytecode::wcSimpleExecContext& output, int startingDepth, vector<wcParseNode>* rpnOutput)
{
	vector<wcParseNode> stack;
	wcToken currentToken,topOfStack;
	bool foundParen = false;

	//increment through nodes creating a RPN expression
	while (p_index.getCurrentNodeDepth() > startingDepth)
	{
		currentToken = p_index.getNode()->tokens[0];	
		switch (p_index.getNode()->type)
		{
		case pn_exp:
			genSimpExpression(p_index, output, rpnOutput);
			break;

		case pn_ident:	case pn_varident:	case pn_funcident:	case pn_funccall:
		CASE_ALL_LITERALS_PN
			rpnOutput->push_back(p_index.getNode().node->data);
			break;

		CASE_ALL_OPERATORS_PN
			while (stack.size() && (getPrecedence(currentToken) == getPrecedence(stack[stack.size() - 1].tokens[0]) && !isRightAssociative(currentToken)
				|| isRightAssociative(currentToken) && getPrecedence(currentToken) < getPrecedence(stack[stack.size() - 1].tokens[0])))
			{
				rpnOutput->push_back(stack[stack.size() - 1]);
				stack.pop_back();
			}
			stack.push_back(p_index.getNode().node->data);
			break;

		case pn_oparen:
			stack.push_back(p_index.getNode().node->data);
			break;

		case pn_cparen:
			while (!foundParen && stack.size())
				if (stack[stack.size() - 1].tokens[0].type == pn_oparen)
					foundParen = true;
				else
				{
					rpnOutput->push_back(stack[stack.size() - 1]);
					stack.pop_back();
				}

			if (!foundParen)
				return *rpnOutput;	//error
			stack.pop_back();

			//func ident? copied from old version
			if (stack.size() && stack[stack.size() - 1].type == pn_funcident)
			{
				rpnOutput->push_back(stack[stack.size() - 1]);
				stack.pop_back();
			}
			break;

		default:

			break;
		}
		p_index.nextNode();
	}

	//pop off remaining operators
	while (stack.size() > 0)
	{
		rpnOutput->push_back(stack[stack.size() - 1]);
		stack.pop_back();
	}

	return *rpnOutput;
}

vector<shared_ptr<wcInstruction>> wc::codegen::genSimpExpression_genInstructionsFromRPN(wcParseIndex& p_index, bytecode::wcSimpleExecContext& output, vector<wcParseNode>* rpnOutput)
{
	vector<shared_ptr<wcInstruction>> instructionOutput;
	int operand1,operand2; 
	string stringLiteral;
	wcInstructionPlusOperand s;
	shared_ptr<wcInstructionPlusOperand> p;

	for (int i = 0; i < rpnOutput->size(); ++i)
		switch(rpnOutput->at(i).type)
		{
		//literals
		case pn_strlit:
			stringLiteral = rpnOutput->at(i).tokens[0].data;
			if (!output.stringTable.doesStringExist(stringLiteral))
				operand1 = output.stringTable.addEntry(stringLiteral);	//add to string table if it's the first occurence
			else
				operand1 = output.stringTable.getIndex(stringLiteral);	//we've seen this before, retrieve index from string table
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, operand1)));
			break;

		case pn_intlit:
			operand1 = stoi(rpnOutput->at(i).tokens[0].data);
			instructionOutput.push_back(make_shared<wcInstructionPlusOperand>(wcInstructionPlusOperand(soc_push, operand1)));
			s = *static_pointer_cast<wcInstructionPlusOperand>(instructionOutput.at(0));
			break;

		case pn_fltlit:
			break;

		case pn_true:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, 1)));
			break;
			
		case pn_false:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, 0)));
			break;

		//variables
		case pn_ident:
		case pn_varident:
		case pn_funcident:
		case pn_funccall:
			break;

		//operators
		case pn_lognot:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_not)));
			break;
		case pn_logand:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_and)));
			break;
		case pn_logor:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_or)));
			break;
		case pn_equal:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_cmp)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_jne, instructionOutput.size() + 3)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, 1)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_jmp, instructionOutput.size() + 2)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, 0)));
			break;
		case pn_notequal:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_cmp)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_je, instructionOutput.size() + 3)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, 1)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_jmp, instructionOutput.size() + 2)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, 0)));
			break;
		case pn_greater:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_cmp)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_jle, instructionOutput.size() + 3)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, 1)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_jmp, instructionOutput.size() + 2)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, 0)));
			break;
		case pn_greaterequal:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_cmp)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_jl, instructionOutput.size() + 3)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, 1)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_jmp, instructionOutput.size() + 2)));
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstructionPlusOperand(soc_push, 0)));
			break;

		case pn_assign:
		case pn_mult:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_mult)));
			break;
		case pn_div:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_div)));
			break;
		case pn_plus:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_plus)));
			break;
		case pn_minus:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_minus)));
			break;
		case pn_mod:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_mod)));
			break;
		case pn_expo:
			instructionOutput.push_back(make_shared<wcInstruction>(wcInstruction(soc_expo)));
			break;
		default:
			break;
		}

	return instructionOutput;
}

int wc::codegen::genSimpIf(wcParseIndex& p_index, bytecode::wcSimpleExecContext& output)
{

	return 1;
}

int wc::codegen::genSimpDecVar(wcParseIndex& p_index, bytecode::wcSimpleExecContext& output)
{
	wcToken typeToken,identToken;
	vector<shared_ptr<wcInstruction>> expressionInstructions;
	int startingDepth = p_index.getNodeDepth(p_index.getNode());
	p_index.nextNode();

	while (p_index.getCurrentNodeDepth() > startingDepth)
		switch (p_index.getNode()->type)
		{
		case pn_type:
			typeToken = p_index.getNode()->tokens[0];
			p_index.nextNode();
			break;

		case pn_ident:
			identToken = p_index.getNode()->tokens[0];
			p_index.nextNode();
			break;

		case pn_exp:
			expressionInstructions = genSimpExpression(p_index, output);
			break;

		default:
			//error
			return 0;
		}

	return 1;
}

int wc::codegen::genSimpStatement(wcParseIndex& p_index, wcSimpleExecContext& output)
{
	if (p_index.getNode()->type != pn_statement)
		return 0;
	p_index.nextNode();

	vector<shared_ptr<wcInstruction>> expInstructions;
	switch (p_index.getNode()->type)
	{
	case pn_exp:
		expInstructions = genSimpExpression(p_index, output);
		output.instructions.insert(output.instructions.end(), expInstructions.begin(), expInstructions.end());
		break;

	case pn_if:
		genSimpIf(p_index, output);
		break;

	case pn_decvar:
		genSimpDecVar(p_index, output);
		break;
	}

	return 1;
}

namespace wc
{
	namespace codegen
	{
		wcParseIndex gen_initParseIndex(wcParseIndex p_pi, wcAST& p_ast);
	}
}

//increment past the head to the first statement
wcParseIndex wc::codegen::gen_initParseIndex(wcParseIndex p_pi, wcAST& p_ast)
{
	p_pi.setNode(&p_ast.parseTree, p_ast.parseTree.begin());

	if (!p_pi.getNode()->type == pn_head)
		return p_pi;
	
	p_pi.nextNode();

	while (p_pi.getNode()->type != pn_statement)
		p_pi.nextNode();
	return p_pi;
}

wcSimpleExecContext wc::codegen::wcSimpleBytecodeGen::genSimple(wcAST& p_ast)
{
	wcSimpleExecContext output;
	output.targetPlatform = ct_simple_bytecode;

	wcParseIndex pindex = gen_initParseIndex(wcParseIndex(), p_ast);

	while (pindex.getNode() != p_ast.parseTree.end())
		genSimpStatement(pindex,output);

	return output;
}

wc::vm::wcSimpleVM::wcSimpleVM()
{

}

int wc::bytecode::getSimpOpCount(wcSimpleOpcode p_oc)
{
	if (wcSimpleOpcodeOperandCounts.find(p_oc) == wcSimpleOpcodeOperandCounts.end())
		return 0;
	return wcSimpleOpcodeOperandCounts.find(p_oc)->second;
}

int wc::vm::wcSimpleVM::exec(int p_handle)
{
	if (!handleExists(p_handle))
		return -1;

	wcSimpleExecContext& con = static_cast<wcSimpleExecContext&>(conPool[p_handle]);

	while (!con.execStopped())
			execInstruction(con, con.getInstr());

	return 0;
}

int wc::vm::wcSimpleVM::execInstruction(wcSimpleExecContext& p_context, shared_ptr<wcInstruction> p_instruction)
{
	switch (p_instruction->opcode)
	{
		case soc_nop:
			break;
		case soc_push:
			exec_s_push(p_context, *static_pointer_cast<wcInstructionPlusOperand>(p_instruction));
			break;
		case soc_pop:
			exec_s_pop(p_context, *p_instruction);
			break;
		case soc_cmp: 
			exec_s_cmp(p_context, *p_instruction);
			break;
		case soc_jmp:
			exec_s_jmp(p_context, *static_pointer_cast<wcInstructionPlusOperand>(p_instruction));
			break;
		case soc_je:
			exec_s_je(p_context, *static_pointer_cast<wcInstructionPlusOperand>(p_instruction));
			break;
		case soc_jne: 
			exec_s_jne(p_context, *static_pointer_cast<wcInstructionPlusOperand>(p_instruction));
			break;
		case soc_jg:
			exec_s_jg(p_context, *static_pointer_cast<wcInstructionPlusOperand>(p_instruction));
			break;
		case soc_jl:
			exec_s_jl(p_context, *static_pointer_cast<wcInstructionPlusOperand>(p_instruction));
			break;
		case soc_jge:
			exec_s_jge(p_context, *static_pointer_cast<wcInstructionPlusOperand>(p_instruction));
			break;
		case soc_jle:
			exec_s_jle(p_context, *static_pointer_cast<wcInstructionPlusOperand>(p_instruction));
			break;
		case soc_assign:
			exec_s_assign(p_context, *static_pointer_cast<wcInstructionPlusOperand>(p_instruction));
			break;
		case soc_plus:
			exec_s_plus(p_context, *p_instruction);
			break;
		case soc_minus:
			exec_s_minus(p_context, *p_instruction);
			break;
		case soc_mult: 
			exec_s_mult(p_context, *p_instruction);
			break;
		case soc_div: 
			exec_s_div(p_context, *p_instruction);
			break;
		case soc_expo: 
			exec_s_expo(p_context, *p_instruction);
			break;
		case soc_mod: 
			exec_s_mod(p_context, *p_instruction);
			break;
		case soc_inc:
			exec_s_inc(p_context, *p_instruction);
			break;
		case soc_dec:
			exec_s_dec(p_context, *p_instruction);
			break;
		case soc_and:
			exec_s_and(p_context, *p_instruction);
			break;
		case soc_or:
			exec_s_or(p_context, *p_instruction);
			break;
		case soc_xor:
			exec_s_xor(p_context, *p_instruction);
			break;
		case soc_not:
			exec_s_not(p_context, *p_instruction);
			break;
		case soc_shfl:
			exec_s_shfl(p_context, *p_instruction);
			break;
		case soc_shfr:
			exec_s_shfr(p_context, *p_instruction);
			break;
		case soc_halt:
			exec_s_halt(p_context, *static_pointer_cast<wcInstructionPlusOperand>(p_instruction));
			break;

	}
	
	p_context.registers.pc++;

	return 1;
}

wc::compile::wcSimpleCompiler::wcSimpleCompiler()
{

}

wcScript wc::compile::wcSimpleCompiler::compile(vector<string> p_source)
{
	wcScript output;

	//lex
	wcLexer lex;
	vector<wcToken> tokens = lex.lex(p_source);
	if (lex.getError().code)
		return wcScript(lex.getError());

	//parse
	wcParser parser;
	wcAST tree = parser.parse(tokens);
	if (parser.getError().code)
		return wcScript(parser.getError());

	//code gen
	wcSimpleBytecodeGen gen;
	output.con = gen.gen(tree);
	if (gen.getError().code)
		return wcScript(gen.getError());

	//return what we've created
	return output;
}

inline void wc::vm::exec_s_push(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	p_context.stack.push(wcChunki(p_instr.operand1));
}

inline void wc::vm::exec_s_pop(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.stack.pop();
}

void wc::vm::exec_s_pushstk(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	wcChunki stackValue = *p_context.stack.peeki(p_instr.operand1);
	p_context.stack.push(stackValue);
}

void wc::vm::exec_s_pushr(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	wcChunki stackValue = *p_context.stack.peeki(p_instr.operand1);
	p_context.stack.push(stackValue);
}

void wc::vm::exec_s_popstk(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	p_context.stack.set(p_instr.operand1, *p_context.stack.pop());
}

void wc::vm::exec_s_popr(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	p_context.registers[p_instr.operand1] = p_context.stack.popi()->i();
}

inline void wc::vm::exec_s_cmp(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.registers.cmp = p_context.registers.t1 - p_context.registers.t2;
}

inline void wc::vm::exec_s_jmp(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	p_context.registers.pc = p_instr.operand1;
}

inline void wc::vm::exec_s_je(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	if(!p_context.registers.cmp)
		p_context.registers.pc = p_instr.operand1;
}

inline void wc::vm::exec_s_jne(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	if (p_context.registers.cmp)
		p_context.registers.pc = p_instr.operand1;
}

inline void wc::vm::exec_s_jg(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	if (p_context.registers.cmp > 0)
		p_context.registers.pc = p_instr.operand1;
}

inline void wc::vm::exec_s_jl(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	if (p_context.registers.cmp < 0)
		p_context.registers.pc = p_instr.operand1;
}

inline void wc::vm::exec_s_jge(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	if (p_context.registers.cmp >= 0)
		p_context.registers.pc = p_instr.operand1;
}

inline void wc::vm::exec_s_jle(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	if (p_context.registers.cmp <= 0)
		p_context.registers.pc = p_instr.operand1;
}

inline void wc::vm::exec_s_assign(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.popi()->i(); //contained the last stack value for the stack index at operand1
	p_context.registers.t2 = p_instr.operand1;	//stackindex
}

inline void wc::vm::exec_s_plus(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1 + p_context.registers.t2));
}

inline void wc::vm::exec_s_minus(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1 - p_context.registers.t2));
}

inline void wc::vm::exec_s_mult(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1 * p_context.registers.t2));
}

inline void wc::vm::exec_s_div(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1 / p_context.registers.t2));
}

inline void wc::vm::exec_s_expo(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1 ^ p_context.registers.t2));
}

inline void wc::vm::exec_s_mod(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1 % p_context.registers.t2));
}

inline void wc::vm::exec_s_inc(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1++));
}

inline void wc::vm::exec_s_dec(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1--));
}

inline void wc::vm::exec_s_and(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1 && p_context.registers.t2));
}

inline void wc::vm::exec_s_or(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1 || p_context.registers.t2));
}

inline void wc::vm::exec_s_xor(wcSimpleExecContext &p_context, wcInstruction p_instr)
{

}

inline void wc::vm::exec_s_not(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(!p_context.registers.t1));
}

inline void wc::vm::exec_s_shfl(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1 << p_context.registers.t2));
}

inline void wc::vm::exec_s_shfr(wcSimpleExecContext &p_context, wcInstruction p_instr)
{
	p_context.registers.t2 = p_context.stack.popi()->i();
	p_context.registers.t1 = p_context.stack.popi()->i();
	p_context.stack.push(wcChunki(p_context.registers.t1 >> p_context.registers.t2));
}

inline void wc::vm::exec_s_halt(wcSimpleExecContext &p_context, wcInstructionPlusOperand p_instr)
{
	p_context.registers.halt = p_instr.operand1;
}

wc::bytecode::wcSimpleExecContext::wcSimpleExecContext()
{
	contextID = 0;
	targetPlatform = wcTargetPlatform::ct_simple_bytecode;
}

wc::bytecode::wcSimpleExecContext::wcSimpleExecContext(wcExecContext && p_context)
{
	contextID = p_context.contextID;
	debugSymbolTable = p_context.debugSymbolTable;
	instructions = p_context.instructions;
	registers = p_context.registers;
	stringTable = p_context.stringTable;
	targetPlatform = p_context.targetPlatform;
}
