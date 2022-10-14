#include "parse.h"
#include "lex.h"

using namespace std;
using namespace weec;
using namespace weec::lex;
using namespace weec::parse;

using enum weec::parse::wcParserErrorCode;
using enum weec::lex::wcTokenType;
using enum weec::parse::wcParseNodeType;

struct wcParserConsts
{
	std::string Parser_GlobalScopeIdent = "$global",
		Parser_ScopeDelimiter = "::";
};

static wcParserConsts ParserConsts;

weec::parse::wcParser::wcParser(lex::wcTokenizer& _Tokenizer) : Tokenizer(_Tokenizer)
{

}

wcParseOutput weec::parse::wcParser::Parse()
{
	wcParseOutput Output;

	while (Tokenizer.NextToken())
	{
		auto ThisToken = Tokenizer.GetToken();

		Output.AddAsChild(ParseStatement());
	}

	Output.SymbolTable = SymbolTable;

	return Output;
}

wcParseOutput weec::parse::wcParser::ParseDeclaration()
{
	return wcParseOutput();
}

wcParseOutput weec::parse::wcParser::ParseDeclaration(wcParseSymbol)
{
	//expect a type
	auto TypeToken = Tokenizer.GetToken();
	if (!TypeToken.IsBuiltinType() && TypeToken.Type != Identifier)
	{
		//error - expected built in or user type
		return wcParseOutput(wcParserError(UnexpectedToken, TypeToken));
	}

	if (!Tokenizer.NextToken())
	{

	}

	//optional assignment

	//optional


	return wcParseOutput();
}

wcParseOutput weec::parse::wcParser::ParseDeclaration_Variable()
{
	return wcParseOutput();
}

wcParseOutput weec::parse::wcParser::ParseDeclaration_Function()
{
	return wcParseOutput();
}

wcParseOutput weec::parse::wcParser::ParseDeclaration_Variable(wcParseSymbol)
{
	return wcParseOutput();
}

weec::parse::wcIdentParser::wcIdentParser(lex::wcTokenizer& _Tokenizer, wcParseSymbolTable& _SymbolTable) : Tokenizer(_Tokenizer), SymbolTable(_SymbolTable)
{

}

wcParseSymbol weec::parse::wcIdentParser::ParseIdent()
{
	auto Ident1 = Tokenizer.GetToken();

	wcFullIdent FullIdent(wcIdent(Ident1.StringToken.Data), wcScope(SymbolTable.GetCurrentScope().FullIdent.Get()));

	if (SymbolTable.Exists(FullIdent))
	{

	}
	wcParseSymbol Symbol(SymbolTable.ClassifyIdent(Ident1).Type, FullIdent);

	Symbol.FullIdent = FullIdent;
	return Symbol;
}

wcParseOutput weec::parse::wcParser::ParseStatement()
{
	wcParseOutput Output;
	auto ThisToken = Tokenizer.GetToken();

	Output.AddAsChild(wcParseNode(wcParseNodeType::Statement), true);

	switch (ThisToken.Type)
	{
	WC_SWITCHCASE_TOKENS_LITERAL
	WC_SWITCHCASE_TOKENS_OPERATORS_ALL
	case OpenParenthesis:
		Output.AddAsChild(wcExpressionParser(Tokenizer, SymbolTable).ParseExpression());
		break;

	WC_SWITCHCASE_TOKENS_BUILTIN_TYPES
		Output.AddAsChild(ParseDeclaration());
		break;

	case Identifier:
		auto Ident = wcIdentParser(Tokenizer, SymbolTable).ParseIdent();
		switch (Ident.Type)
		{
		case wcParseSymbolType::Type:
			Output.AddAsChild(ParseDeclaration(Ident));

		case wcParseSymbolType::Variable:
		case wcParseSymbolType::Function:
			Output.AddAsChild(wcExpressionParser(Tokenizer, SymbolTable).ParseExpression(Ident));
			if (Tokenizer.GetToken().Type != SemiColon)
			{
				//error
				Output.Error = wcParserError(wcParserErrorCode::UnexpectedToken, Tokenizer.GetToken());
				return Output;
			}
			Tokenizer.NextToken();
			break;

		case wcParseSymbolType::Namespace:
		case wcParseSymbolType::Null:
		default:
			//error
			break;
		}
		break;

	}

	Tokenizer.NextToken();

	return Output;
}

tree<wcParseNode>::pre_order_iterator weec::parse::wcParseExpression::GetExpressionRootNodeBegin()
{
	auto it = AST.begin();
	while (it != AST.end())
	{
		if (it->Type == Expression)
			return it;
		it++;
	}
	return AST.end();
}

tree<wcParseNode>::pre_order_iterator weec::parse::wcParseExpression::GetExpressionRootNodeEnd()
{
	auto it = GetExpressionNodeBegin();
	unsigned int StartingDepth = AST.depth(it);
	while (AST.depth(it) > StartingDepth)
		it++;
	return it;
}

tree<wcParseNode>::pre_order_iterator weec::parse::wcParseExpression::GetExpressionNodeEnd()
{
	return GetExpressionRootNodeEnd()--;
}

void weec::parse::wcParseExpression::Eval()
{
	//tree<wcParseNode>::iterator
}

tree<wcParseNode>::pre_order_iterator weec::parse::wcParseExpression::GetExpressionNodeBegin()
{
	auto t = GetExpressionRootNodeBegin();
	t++;
	return t;
}

weec::parse::wcParseExpression::wcParseExpression()
{
	auto ExpNode = AST.insert(AST.begin(), *new wcParseNode(Expression));
}

weec::parse::wcParseExpression::wcParseExpression(wcParseNodeType HeadType, lex::wcToken OperatorOrLiteral)
{
	auto ExpNode = AST.insert(AST.begin(), *new wcParseNode(Expression));

	AST.append_child(ExpNode, *new wcParseNode(HeadType, OperatorOrLiteral));

	Tokens.push_back(OperatorOrLiteral);
}

weec::parse::wcParseExpression::wcParseExpression(wcParseExpression& OtherExpression)
{
	AST = tree<wcParseNode>(OtherExpression.AST);

	for (auto t = OtherExpression.Tokens.begin(); t != OtherExpression.Tokens.end(); ++t)
		Tokens.push_back(*t);
}

weec::parse::wcParseExpression::wcParseExpression(wcParseNodeType HeadType, wcParseExpression LeftHand, wcToken Operator, wcParseExpression RightHand)
{
	//build the ast
	auto ExpRootNode = AST.insert(AST.begin(), *new wcParseNode(Expression));

	auto OpNode = AST.append_child(ExpRootNode, wcParseNode(HeadType, Operator));
	auto OpNodeChild = AST.append_child(OpNode);

	AST.insert_subtree(OpNodeChild, LeftHand.GetExpressionNodeBegin());
	AST.insert_subtree(OpNodeChild, RightHand.GetExpressionNodeBegin());
	AST.erase(OpNodeChild);

	//tokens in order
	for (auto t = LeftHand.Tokens.begin(); t != LeftHand.Tokens.end(); ++t)
		Tokens.push_back(*t);
	Tokens.push_back(Operator);
	for (auto t = RightHand.Tokens.begin(); t != RightHand.Tokens.end(); ++t)
		Tokens.push_back(*t);
}

weec::parse::wcParseExpression::wcParseExpression(lex::wcToken Operator, wcParseExpression RightHand)
{
	//AST.set_head(*new wcParseNode(wcParseNodeType::Head));

	Tokens.push_back(Operator);

	for (auto t = RightHand.Tokens.begin(); t != RightHand.Tokens.end(); ++t)
		Tokens.push_back(*t);
}


wcParseOutput weec::parse::wcExpressionParser::ParseExpression()
{
	auto Expression = ParseExpression_Expression();
	wcParseOutput Output;
	Output.AST = Expression.AST;
	return Output;
}
wcParseOutput weec::parse::wcExpressionParser::ParseExpression(wcParseSymbol Symbol)
{
	auto Expression = ParseExpression_Expression();
	wcParseOutput Output;
	Output.AST = Expression.AST;
	return Output;
}

wcParseExpression weec::parse::wcExpressionParser::ParseExpression_Expression()
{
	wcParseExpression Expression1;

	Expression1 = ParseExpression_SubExpression();

	Expression1.Eval();

	return Expression1;
}

wcParseExpression weec::parse::wcExpressionParser::ParseExpression_SubExpression()
{
	auto Expression = ParseExpression_Equality();

	return Expression;
}

wcParseExpression weec::parse::wcExpressionParser::ParseExpression_Equality()
{
	wcParseExpression Output = ParseExpression_Assignment();

	auto Operator = Tokenizer.GetToken();
	while (Operator.Type == NotEqualOperator || Operator.Type == EqualOperator)
	{
		Tokenizer.NextToken();
		auto RightExp = ParseExpression_Assignment();
		Output = wcParseExpression(Expression_Equality, Output, Operator, RightExp);

		Operator = Tokenizer.GetToken();
	}

	return Output;
}

wcParseExpression weec::parse::wcExpressionParser::ParseExpression_Assignment()
{
	wcParseExpression Output = ParseExpression_LogicOr();

	auto Operator = Tokenizer.GetToken();
	while (Operator.Type == AssignOperator)
	{
		Tokenizer.NextToken();
		auto RightExp = ParseExpression_LogicOr();
		Output = wcParseExpression(Expression_Assignment, Output, Operator, RightExp);

		Operator = Tokenizer.GetToken();
	}

	return Output;
}

wcParseExpression weec::parse::wcExpressionParser::ParseExpression_LogicOr()
{
	wcParseExpression Output = ParseExpression_LogicAnd();

	auto Operator = Tokenizer.GetToken();
	while (Operator.Type == wcTokenType::LogOrOperator)
	{
		Tokenizer.NextToken();
		auto RightExp = ParseExpression_LogicAnd();
		Output = wcParseExpression(Expression_LogicOr, Output, Operator, RightExp);

		Operator = Tokenizer.GetToken();
	}

	return Output;
}
wcParseExpression weec::parse::wcExpressionParser::ParseExpression_LogicAnd()
{
	wcParseExpression Output = ParseExpression_Comparison();

	auto Operator = Tokenizer.GetToken();
	while (Operator.Type == wcTokenType::LogAndOperator)
	{
		Tokenizer.NextToken();
		auto RightExp = ParseExpression_Comparison();
		Output = wcParseExpression(Expression_LogicAnd, Output, Operator, RightExp);

		Operator = Tokenizer.GetToken();
	}

	return Output;
}

wcParseExpression weec::parse::wcExpressionParser::ParseExpression_Comparison()
{
	wcParseExpression Output = ParseExpression_Term();

	auto Operator = Tokenizer.GetToken();
	while (Operator.Type == GreaterEqualOperator || Operator.Type == GreaterOperator ||
		Operator.Type == LessOperator || Operator.Type == LessEqualOperator)
	{
		Tokenizer.NextToken();
		auto RightExp = ParseExpression_Term();
		Output = wcParseExpression(Expression_Comparison, Output, Operator, RightExp);

		Operator = Tokenizer.GetToken();
	}

	return Output;
}

wcParseExpression weec::parse::wcExpressionParser::ParseExpression_Term()
{
	wcParseExpression Output = ParseExpression_Factor();

	auto Operator = Tokenizer.GetToken();
	while (Operator.Type == MinusOperator || Operator.Type == PlusOperator)
	{
		Tokenizer.NextToken();
		auto RightExp = ParseExpression_Factor();
		Output = wcParseExpression(Expression_Term, Output, Operator, RightExp);

		Operator = Tokenizer.GetToken();
	}

	return Output;
}

wcParseExpression weec::parse::wcExpressionParser::ParseExpression_Factor()
{
	wcParseExpression Output = ParseExpression_Unary();

	auto Operator = Tokenizer.GetToken();
	while (Operator.Type == DivideOperator || Operator.Type == MultiplyOperator)
	{
		Tokenizer.NextToken();
		auto RightExp = ParseExpression_Unary();
		Output = wcParseExpression(Expression_Factor, Output, Operator, RightExp);

		Operator = Tokenizer.GetToken();
	}

	return Output;
}

wcParseExpression weec::parse::wcExpressionParser::ParseExpression_Unary()
{
	wcParseExpression Output = ParseExpression_Primary();

	auto Operator = Tokenizer.GetToken();
	while (Operator.Type == MinusOperator || Operator.Type == LogNotOperator)
	{
		Tokenizer.NextToken();
		auto RightExp = ParseExpression_Primary();
		Output = wcParseExpression(Expression_Primary, Output, Operator, RightExp);

		Operator = Tokenizer.GetToken();
	}

	return Output;
}

wcParseExpression weec::parse::wcExpressionParser::ParseExpression_Primary()
{
	wcParseExpression Output;

	auto Operand = Tokenizer.GetToken();
	switch (Operand.Type)
	{
	case TrueKeyword:
	case FalseKeyword:
	case StringLiteral:
	case FloatLiteral:
	case CharLiteral:
	case IntLiteral:
	case Identifier:
		Output = wcParseExpression(Expression_Primary, Operand);
		Tokenizer.NextToken();
		return Output;

	case OpenParenthesis:
		//consume opening parenthesis
		Tokenizer.NextToken();

		Output = ParseExpression_SubExpression();

		//consume closing parenthesis
		if (Tokenizer.GetToken().Type != CloseParenthesis)
		{
			//didnt find one, madness
			return Output;
		}
		Tokenizer.NextToken();
		return Output;

	default:
		//error
		break;
	}

	//error
	return Output;
}

weec::parse::wcParseOutput::wcParseOutput()
{
	NodeIndex = UnofficialHead = AST.insert(AST.begin(), wcParseNode(wcParseNodeType::Head));
}

weec::parse::wcParseOutput::wcParseOutput(wcParserError _Error)
{
	NodeIndex = UnofficialHead = AST.insert(AST.begin(), wcParseNode(wcParseNodeType::Head));
	Error = _Error;
}

void weec::parse::wcParseOutput::AddAsChild(wcParseOutput Child, bool PointToChild)
{
	if (PointToChild)
		NodeIndex = AST.append_child(NodeIndex, Child.AST.begin());
	else
		AST.append_child(NodeIndex, Child.AST.begin());
}

void weec::parse::wcParseOutput::AddAsChild(wcParseNode Node, bool PointToChild)
{
	auto t = NodeIndex;

	if (PointToChild)
		NodeIndex = AST.append_child(NodeIndex, Node);
	else
		AST.append_child(NodeIndex, Node);
}


//append as child
wcParseOutput weec::parse::wcParseOutput::operator+(wcParseOutput other)
{
	AST.insert_subtree(AST.begin(), other.AST.begin());

	return *this;
}

wcParseOutput weec::parse::wcParseOutput::operator=(wcParseOutput other)
{
	AST = other.AST;

	return *this;
}

wcParseOutput weec::parse::wcParseOutput::operator+=(wcParseOutput other)
{
	*this = *this + other;

	return *this;
}

weec::parse::wcParseNode::wcParseNode()
{
	Type = wcParseNodeType::Empty;
	Token = wcToken();
}

weec::parse::wcParseNode::wcParseNode(const wcParseNode& Other)
{
	Token = Other.Token;
	Type = Other.Type;
}

weec::parse::wcParseNode::wcParseNode(wcParseNodeType _Type)
{
	Type = _Type;
}

weec::parse::wcParseNode::wcParseNode(wcParseNodeType _Type, wcToken _Token)
{
	Type = _Type;
	Token = _Token;
}

weec::parse::wcParseNode::wcParseNode(lex::wcToken _Token)
{
	Type = wcParseNodeType::Expression_Operator;	//assumption
	Token = _Token;
}

bool weec::parse::wcParseNode::operator==(const wcParseNode& Other)
{
	if (Other.Token == Token && Other.Type == Type)
		return true;
	return false;
}

bool weec::parse::wcParseNode::operator==(wcParseNode Other)
{
	if (Other.Token == Token && Other.Type == Type)
		return true;
	return false;
}

wcParseNode& weec::parse::wcParseNode::operator=(const wcParseNode& Other)
{
	Token = Other.Token;
	Type = Other.Type;
	return *this;
}

std::string weec::parse::wcParseNodeTypeToString(wcParseNodeType Type)
{
	switch (Type)
	{
	case wcParseNodeType::Head:
		return "Head";
	case wcParseNodeType::Empty:
		return "Empty";
	case wcParseNodeType::Statement:
		return "Statement";
	case wcParseNodeType::Variable:
		return "Variable";
	case wcParseNodeType::Type:
		return "Type";
	case wcParseNodeType::Expression:
		return "Expression";
	case wcParseNodeType::Expression_Assignment:
		return "Expression_Assignment";
	case wcParseNodeType::Expression_Comparison:
		return "Expression_Comparison";
	case wcParseNodeType::Expression_Equality:
		return "Expression_Equality";
	case wcParseNodeType::Expression_Factor:
		return "Expression_Factor";
	case wcParseNodeType::Expression_LogicAnd:
		return "Expression_LogicAnd";
	case wcParseNodeType::Expression_LogicOr:
		return "Expression_LogicOr";
	case wcParseNodeType::Expression_Primary:
		return "Expression_Primary";
	case wcParseNodeType::Expression_Term:
		return "Expression_Term";
	case wcParseNodeType::Expression_Unary:
		return "Expression_Unary";
	case wcParseNodeType::Expression_Operator:
		return "Expression_Operator";
	default:
		return "";
	}
}

wcParseNodeType wcTokenTypeToParseNodeType(wcTokenType Type)
{
	switch (Type)
	{
	case wcTokenType::Null:
		return wcParseNodeType::Head;

	default:
		return wcParseNodeType::Head;
	}
}

weec::parse::wcParseSymbolTable::wcParseSymbolTable() : NullSymbol(wcParseSymbolType::Null, wcFullIdent("", ""))
{
	//global scope
	Add(wcParseSymbolType::Type, wcFullIdent(ParserConsts.Parser_GlobalScopeIdent, ParserConsts.Parser_GlobalScopeIdent), true);

	//built-in types
	Add(wcParseSymbolType::Type, wcFullIdent("void", ParserConsts.Parser_GlobalScopeIdent));
	Add(wcParseSymbolType::Type, wcFullIdent("int", ParserConsts.Parser_GlobalScopeIdent));
	Add(wcParseSymbolType::Type, wcFullIdent("uint", ParserConsts.Parser_GlobalScopeIdent));
	Add(wcParseSymbolType::Type, wcFullIdent("float", ParserConsts.Parser_GlobalScopeIdent));
	Add(wcParseSymbolType::Type, wcFullIdent("string", ParserConsts.Parser_GlobalScopeIdent));
}

wcParseSymbolTable& weec::parse::wcParseSymbolTable::operator=(const wcParseSymbolTable& Other)
{
	Container = Other.Container;
	this->CurrentScopeFullIdent = Other.CurrentScopeFullIdent; 
	return *this;
}

bool weec::parse::wcParseSymbolTable::Add(wcParseSymbolType Type, wcIdent Ident)
{
	wcFullIdent FullIdent = wcFullIdent(Ident, wcScope(GetCurrentScope().FullIdent.Get()));
	wcParseSymbol Symbol = wcParseSymbol(Type, FullIdent);

	if (Exists(FullIdent))
		return false;

	Container.insert(make_pair(FullIdent.Get(), Symbol));

	SetScope(FullIdent);

	return true;
}

bool weec::parse::wcParseSymbolTable::Add(wcParseSymbolType Type, wcFullIdent FullIdent, bool PointScopeToThis)
{
	wcParseSymbol Symbol = wcParseSymbol(Type, FullIdent);

	if (Exists(FullIdent))
		return false;

	Container.insert(make_pair(FullIdent.Get(), Symbol));

	if (PointScopeToThis)
		SetScope(FullIdent);

	return true;
}

bool weec::parse::wcParseSymbolTable::Exists(wcFullIdent FullIdent) const
{
	if (Container.find(FullIdent.Get()) != Container.end())
		return true;
	else
		return false;
}

wcParseSymbol weec::parse::wcParseSymbolTable::Get(wcFullIdent FullIdent) const
{
	if (Container.find(FullIdent.Get()) != Container.end())
		return Container.at(FullIdent.Get());
	else
		return NullSymbol;
}

bool weec::parse::wcParseSymbolTable::SetScope(wcFullIdent FullIdent)
{
	if (!Exists(FullIdent))
		return false;

	CurrentScopeFullIdent = Get(FullIdent);

	return true;
}

bool weec::parse::wcParseSymbolTable::SetScope(wcParseSymbol& Symbol)
{
	if (!Exists(Symbol.FullIdent))
		return false;
	CurrentScopeFullIdent = Get(Symbol.FullIdent);
	return true;
}

wcParseSymbol  weec::parse::wcParseSymbolTable::GetCurrentScope() const
{
	return CurrentScopeFullIdent;
}

wcParseSymbol weec::parse::wcParseSymbolTable::ClassifyIdent(lex::wcToken Ident)
{
	wcFullIdent FullIdent;

	//check if ident is fully qualified
	if (wcTokenTypeAlizer().IsIdentQualified(Ident.StringToken.Data))
		FullIdent = wcFullIdent(Ident.StringToken.Data);
	else
		FullIdent = wcFullIdent(wcIdent(Ident.StringToken.Data), wcScope(GetCurrentScope().FullIdent.Get()));

	if (Exists(FullIdent))
	{

	}

	return wcParseSymbol(wcParseSymbolType::Variable, FullIdent);
}

weec::parse::wcExpressionParser::wcExpressionParser(lex::wcTokenizer& _Tokenizer, wcParseSymbolTable& _SymbolTable) : Tokenizer(_Tokenizer), SymbolTable(_SymbolTable)
{

}


weec::parse::wcScope::wcScope()
{
}

weec::parse::wcScope::wcScope(const wcScope& Other)
{
	FullIdent = Other.FullIdent;
}

weec::parse::wcScope::wcScope(std::string _Data)
{
	FullIdent = _Data;
}

weec::parse::wcIdent::wcIdent()
{
}

weec::parse::wcIdent::wcIdent(const wcIdent& Other) : Ident(Other.Ident)
{
	Ident = Other.Ident;
}

weec::parse::wcIdent::wcIdent(std::string _Data)
{
	Ident = _Data;
}

weec::parse::wcFullIdent::wcFullIdent(std::string FullIdent)
{
	if (FullIdent.find("::") == std::string::npos && FullIdent.find("."))
	{
		//not qualified as expected, global scope
		Ident = wcIdent(FullIdent);
		Scope = wcScope(ParserConsts.Parser_GlobalScopeIdent);
		return;
	}

	//get scope
	Scope = wcScope(FullIdent.substr(FullIdent.find_last_of("::")));

}

weec::parse::wcFullIdent::wcFullIdent(std::string _Ident, std::string _Scope)
{
	Ident = wcIdent(_Ident);
	Scope = wcScope(_Scope);
}

weec::parse::wcFullIdent::wcFullIdent(wcIdent _Ident, wcScope _Scope)
{
	Ident = wcIdent(_Ident);
	Scope = wcScope(_Scope);
}

weec::parse::wcFullIdent::wcFullIdent()
{
}

weec::parse::wcFullIdent::wcFullIdent(const wcFullIdent& Other)
{
	Ident = wcIdent(Other.Ident);
	Scope = wcScope(Other.Scope);
}

std::string weec::parse::wcFullIdent::Get() const
{
	if (Ident.Get() == ParserConsts.Parser_GlobalScopeIdent)
		return Ident.Get();		//stops the global scope FullIdent being global::global
	return Scope.Get() + ParserConsts.Parser_ScopeDelimiter.c_str() + Ident.Get();
}

weec::parse::wcParseSymbol::wcParseSymbol() : FullIdent()
{
	Type = wcParseSymbolType::Null;
}

weec::parse::wcParseSymbol::wcParseSymbol(const wcParseSymbol& Other)
{
	Type = Other.Type;
	FullIdent = Other.FullIdent;
}

weec::parse::wcParseSymbol::wcParseSymbol(wcParseSymbolType _Type, wcFullIdent _FullIdent) : FullIdent(_FullIdent)
{
	FullIdent = wcFullIdent(_FullIdent);
	Type = _Type;
}
