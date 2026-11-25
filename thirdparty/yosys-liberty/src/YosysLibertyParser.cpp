/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "YosysLibertyParser.h"

#include <istream>
#include <sstream>
#include "YosysLibertyException.h"

namespace Yosys {

namespace {

bool contains(const std::set<std::string> &names, const std::string &name)
{
	return names.find(name) != names.end();
}

}

LibertyAst::~LibertyAst()
{
  for (auto child : children)
    delete child;
  children.clear();
}

LibertyAst *LibertyAst::find(std::string name)
{
  for (auto child : children)
    if (child->id == name)
      return child;
  return NULL;
}

void LibertyParser::error() {
  std::ostringstream reason;
  reason << "LibertyParser error at line " << line;
  throw naja::liberty::YosysLibertyException(reason.str());
}

void LibertyParser::error(const std::string &str) {
  std::ostringstream reason;
  reason << "LibertyParser error at line " << line << ": " << str;
  throw naja::liberty::YosysLibertyException(reason.str());
}

int LibertyParser::lexer(std::string &str)
{
	int c;

	// eat whitespace
	do {
		c = f.get();
	} while (c == ' ' || c == '\t' || c == '\r');

	// search for identifiers, numbers, plus or minus.
	if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_' || c == '-' || c == '+' || c == '.') {
		str = static_cast<char>(c);
		while (1) {
			c = f.get();
			if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_' || c == '-' || c == '+' || c == '.')
				str += c;
			else
				break;
		}
		f.unget();
		if (str == "+" || str == "-") {
			/* Single operator is not an identifier */
			// fprintf(stderr, "LEX: char >>%s<<\n", str.c_str());
			return str[0];
		}
		else {
			// fprintf(stderr, "LEX: identifier >>%s<<\n", str.c_str());
			return 'v';
		}
	}

	// if it wasn't an identifer, number of array range,
	// maybe it's a string?
	if (c == '"') {
		str = "";
		while (1) {
			c = f.get();
			if (c == '\n')
				line++;
			if (c == '"')
				break;
			str += c;
		}
		// fprintf(stderr, "LEX: string >>%s<<\n", str.c_str());
		return 'v';
	}

	// if it wasn't a string, perhaps it's a comment or a forward slash?
	if (c == '/') {
		c = f.get();
		if (c == '*') {         // start of '/*' block comment
			int last_c = 0;
			while (c > 0 && (last_c != '*' || c != '/')) {
				last_c = c;
				c = f.get();
				if (c == '\n')
					line++;
			}
			return lexer(str);
		} else if (c == '/') {  // start of '//' line comment
			while (c > 0 && c != '\n')
				c = f.get();
			line++;
			return lexer(str);
		}
		f.unget();
		// fprintf(stderr, "LEX: char >>/<<\n");
		return '/';             // a single '/' charater.
	}

	// check for a backslash
	if (c == '\\') {
		c = f.get();		
		if (c == '\r')
			c = f.get();
		if (c == '\n') {
			line++;
			return lexer(str);
		}
		f.unget();
		return '\\';
	}

	// check for a new line
	if (c == '\n') {
		line++;
		return 'n';
	}

	// anything else, such as ';' will get passed
	// through as literal items.

	// if (c >= 32 && c < 255)
	// 	fprintf(stderr, "LEX: char >>%c<<\n", c);
	// else
	// 	fprintf(stderr, "LEX: char %d\n", c);
	return c;
}

bool LibertyParser::shouldSkipStructuralChildren(const LibertyAst *ast) const
{
	if (parseMode != ParseMode::Structural)
		return false;

	/*
	 * Structural Liberty mode is used by Naja primitive loading: keep the
	 * hierarchy needed to build terms and lightweight modeling, but do not
	 * materialize timing/power/LUT payloads. Groups not listed here are skipped
	 * as soon as their opening '{' is read. If Naja later needs one of these
	 * payloads, uncomment or add that group here, then whitelist its useful
	 * children in shouldKeepStructuralChild().
	 */
	static const std::set<std::string> groupsWithUsefulBodies = {
		"library",
		"type",
		"cell",
		"test_cell",
		"pin",
		"bus",
		"bundle",
		"ff",
		"ff_bank",
		"latch",
		"latch_bank",
		"memory",
		"memory_read",
		"memory_write",
		"timing",

		// "lu_table_template",
		// "power_lut_template",
		// "cell_rise",
		// "cell_fall",
		// "rise_transition",
		// "fall_transition",
		// "rise_power",
		// "fall_power",
		// "internal_power",
		// "leakage_power",
	};

	return !contains(groupsWithUsefulBodies, ast->id);
}

bool LibertyParser::shouldKeepStructuralChild(const LibertyAst *parent, const LibertyAst *child) const
{
	if (parseMode != ParseMode::Structural)
		return true;
	if (parent == nullptr || child == nullptr)
		return true;

	const std::string &parentId = parent->id;
	const std::string &childId = child->id;

	if (parentId == "library")
		return childId == "type" || childId == "cell";

	if (parentId == "type")
		return childId == "bit_from" || childId == "bit_to" || childId == "downto";

	if (parentId == "cell")
		return childId == "pin" || childId == "bus" || childId == "bundle" ||
				childId == "ff" || childId == "ff_bank" ||
				childId == "latch" || childId == "latch_bank" ||
				childId == "memory" || childId == "statetable" || childId == "test_cell";

	if (parentId == "test_cell")
		return childId == "pin" || childId == "ff";

	if (parentId == "pin")
		return childId == "direction" || childId == "function" ||
				childId == "state_function" || childId == "nextstate_type" ||
				childId == "signal_type" ||
				childId == "memory_read" || childId == "memory_write" ||
				childId == "timing";

	if (parentId == "bus")
		return childId == "direction" || childId == "bus_type" ||
				childId == "function" || childId == "state_function" ||
				childId == "pin" ||
				childId == "memory_read" || childId == "memory_write" ||
				childId == "timing";

	if (parentId == "bundle")
		return childId == "direction" || childId == "members" ||
				childId == "function" ||
				childId == "pin" || childId == "bus" || childId == "bundle";

	if (parentId == "timing")
		return childId == "related_pin" || childId == "timing_type";

	if (parentId == "memory")
		return childId == "address_width" || childId == "word_width";

	if (parentId == "memory_read")
		return childId == "address";

	if (parentId == "memory_write")
		return childId == "address" || childId == "clocked_on";

	if (parentId == "ff" || parentId == "ff_bank")
		return childId == "clocked_on" || childId == "next_state" ||
				childId == "clear" || childId == "preset" ||
				childId == "clear_preset_var1";

	if (parentId == "latch" || parentId == "latch_bank")
		return childId == "enable" || childId == "data_in" ||
				childId == "clear" || childId == "preset";

	return false;
}

void LibertyParser::skipGroupBody()
{
	std::string str;
	int depth = 1;

	while (depth > 0) {
		int tok = lexer(str);
		if (tok < 0)
			error("Unexpected end of file while skipping Liberty group.");
		if (tok == '{')
			depth++;
		else if (tok == '}')
			depth--;
	}
}

LibertyAst *LibertyParser::parse()
{
	std::string str;

	int tok = lexer(str);

	// there are liberty files in the wild that
	// have superfluous ';' at the end of
	// a  { ... }. We simply ignore a ';' here.
	// and get to the next statement.

	while ((tok == 'n') || (tok == ';'))
		tok = lexer(str);

	if (tok == '}' || tok < 0)
		return NULL;

	if (tok != 'v') {
		std::string eReport;
		switch(tok)
		{
		case 'n':
			error("Unexpected newline.");
			break;
		case '[':
		case ']':
		case '}':
		case '{':
		case '\"':
		case ':':
			eReport = "Unexpected '";
			eReport += static_cast<char>(tok);
			eReport += "'.";
			error(eReport);
			break;
		default:
			error();
		}
	}

	LibertyAst *ast = new LibertyAst;
	ast->id = str;
	ast->line = line;

	while (1)
	{
		tok = lexer(str);

		// allow both ';' and new lines to 
		// terminate a statement.
		if ((tok == ';') || (tok == 'n'))
			break;

		if (tok == ':' && ast->value.empty()) {
			// Compatibility note:
			// Some Liberty files in the wild use unquoted boolean expressions in
			// key/value pairs (for example: `enable : (clk&a);`) even though the
			// strict Liberty style often uses quoted strings for such expressions.
			// We therefore accept a broader operator/token set here and consume the
			// value until ';' or end-of-line, so these libraries can still be parsed.
			tok = lexer(str);
			while ((tok != ';') && (tok != 'n')) {
				if (tok == 'v') {
					ast->value += str;
				} else if (tok == '+' || tok == '-' || tok == '*' || tok == '/' || tok == '!' ||
				           tok == '&' || tok == '|' || tok == '^' ||
				           tok == '(' || tok == ')' || tok == '\'' ||
				           tok == '<' || tok == '>' || tok == '=' ||
				           tok == '?' || tok == ':' || tok == '%' ||
				           tok == '[' || tok == ']') {
					ast->value += static_cast<char>(tok);
				} else {
					delete ast;
					error();
				}
				tok = lexer(str);
			}

			// In a liberty file, all key : value pairs should end in ';'
			// However, there are some liberty files in the wild that
			// just have a newline. We'll be kind and accept a newline
			// instead of the ';' too..
			if ((tok == ';') || (tok == 'n'))
				break;
			else {
				delete ast;
				error();
			}
			continue;
		}

		if (tok == '(') {
			bool canExtendArgument = false;
			bool needsArgumentContinuation = false;
			while (1) {
				std::string arg;
				tok = lexer(arg);
				if (tok == ',') {
					if (needsArgumentContinuation) {
						delete ast;
						error("Expected an identifier after punctuation in an argument.");
					}
					canExtendArgument = false;
					continue;
				}
				if (tok == ')') {
					if (needsArgumentContinuation) {
						delete ast;
						error("Expected an identifier after punctuation in an argument.");
					}
					break;
				}

				// Some Liberty producers use punctuation in unquoted CCB names and
				// references (for example, `input_ccb(cell_cond__!A&!B)` or
				// `input_ccb(example:a)`). Keep these characters as lexer tokens so
				// they can retain their normal meaning in attributes and vector
				// ranges, and combine them only within an argument here.
				if ((tok == ':' || tok == '!' || tok == '&') && canExtendArgument) {
					ast->args.back() += static_cast<char>(tok);
					needsArgumentContinuation = true;
					continue;
				}
				
				// FIXME: the AST needs to be extended to store
				//        these vector ranges.
				if (tok == '[')
				{
					canExtendArgument = false;
					// parse vector range [A] or [A:B]
					std::string arg;
					tok = lexer(arg);
					if (tok != 'v')
					{
						// expected a vector array index
						delete ast;
						error("Expected a number.");
					}
					else
					{
						// fixme: check for number A
					}
					tok = lexer(arg);
					// optionally check for : in case of [A:B]
					// if it isn't we just expect ']'
					// as we have [A]
					if (tok == ':')
					{
						tok = lexer(arg);
						if (tok != 'v')
						{
							// expected a vector array index
							delete ast;
							error("Expected a number.");
						}
						else
						{
							// fixme: check for number B
							tok = lexer(arg);                            
						}
					}
					// expect a closing bracket of array range
					if (tok != ']')
					{
						delete ast;
						error("Expected ']' on array range.");
					}
					continue;           
				}
				if (tok != 'v') {
					std::string eReport;
					switch(tok)
					{
					case 'n':
					  continue;
					case '[':
					case ']':
					case '}':
					case '{':
					case '\"':
					case ':':
						eReport = "Unexpected '";
						eReport += static_cast<char>(tok);
						eReport += "'.";
						delete ast;
						error(eReport);
						break;
					default:
						delete ast;
						error();
					}
				}
				if (needsArgumentContinuation) {
					ast->args.back() += arg;
					needsArgumentContinuation = false;
				} else {
					ast->args.push_back(arg);
				}
				canExtendArgument = true;
			}
			continue;
		}

		if (tok == '{') {
			if (shouldSkipStructuralChildren(ast)) {
				skipGroupBody();
				break;
			}
			while (1) {
				LibertyAst *child = parse();
				if (child == NULL)
					break;
				if (!shouldKeepStructuralChild(ast, child)) {
					delete child;
					continue;
				}
				ast->children.push_back(child);
			}
			break;
		}
		delete ast;
		error();
	}

	return ast;
}

}
