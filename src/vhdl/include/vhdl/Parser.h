// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vhdl/Lexer.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vhdl {

struct Name {
    std::string spelling;
    std::string canonical;
    SourceSpan span;
};

enum class PortMode { In, Out, InOut, Buffer, Linkage };

struct Expression {
    enum class Kind {
        Name,
        IntegerLiteral,
        RealLiteral,
        StringLiteral,
        CharacterLiteral,
        Unary,
        Binary,
        Conditional,
        Indexed,
        Others,
        Aggregate,
        Range,
        BitStringLiteral,
        Call,
        Selected,
        Association,
        Attribute
    };
    std::vector<std::unique_ptr<Expression>> elements;
    Kind kind;
    std::string text;
    std::string canonical;
    SourceSpan span;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    std::unique_ptr<Expression> condition;
};

struct DiscreteRange {
    std::int64_t left = 0;
    std::int64_t right = 0;
    bool ascending = false;
    SourceSpan span;
    std::shared_ptr<Expression> leftExpression;
    std::shared_ptr<Expression> rightExpression;
    std::shared_ptr<Expression> attribute;
    std::optional<Name> subtype;
};

struct TypeMark {
    Name name;
    std::optional<DiscreteRange> constraint;
};

struct SubtypeDeclaration {
    Name name;
    TypeMark type;
    SourceSpan span;
};

struct AliasDeclaration {
    Name name;
    std::optional<TypeMark> type;
    std::unique_ptr<Expression> target;
    SourceSpan span;
};

struct PortDeclaration {
    std::vector<Name> names;
    PortMode mode = PortMode::In;
    TypeMark type;
    SourceSpan span;
    std::unique_ptr<Expression> defaultValue;
};

struct GenericDeclaration {
    std::vector<Name> names;
    TypeMark type;
    std::unique_ptr<Expression> defaultValue;
    SourceSpan span;
    PortMode mode = PortMode::In;
};

struct ObjectDeclaration {
    std::vector<Name> names;
    TypeMark type;
    SourceSpan span;
    std::unique_ptr<Expression> initializer;
};

using SignalDeclaration = ObjectDeclaration;
using VariableDeclaration = ObjectDeclaration;

struct ConstantDeclaration {
    ObjectDeclaration object;
    std::unique_ptr<Expression> value;
};

struct ArrayTypeDeclaration {
    Name name;
    DiscreteRange indexRange;
    TypeMark elementType;
    SourceSpan span;
    std::optional<Name> indexSubtype;
    std::optional<Name> indexType;
};

struct EnumerationTypeDeclaration {
    Name name;
    std::vector<Name> literals;
    SourceSpan span;
};

struct RecordTypeDeclaration {
    Name name;
    std::vector<ObjectDeclaration> fields;
    SourceSpan span;
};

enum class AssignmentKind { Signal, Variable };

struct Assignment {
    Name target;
    std::unique_ptr<Expression> value;
    SourceSpan span;
    AssignmentKind kind = AssignmentKind::Signal;
    std::vector<std::unique_ptr<Expression>> indices;
    std::unique_ptr<Expression> selector;
    std::vector<std::vector<std::unique_ptr<Expression>>> choices;
    std::vector<std::unique_ptr<Expression>> alternatives;
};

struct SequentialStatement {
    enum class Kind { Assignment, If, For, Return, Case, Null, Exit, While, Wait, Call };
    Kind kind = Kind::Assignment;
    Assignment assignment;
    std::unique_ptr<Expression> condition;
    Name iterator;
    DiscreteRange range;
    std::vector<SequentialStatement> statements;
    std::vector<SequentialStatement> alternative;
    SourceSpan span;
    std::unique_ptr<Expression> returnValue;
    std::unique_ptr<Expression> procedureCall;
    // An empty choice list denotes the final others alternative.
    std::vector<std::vector<std::unique_ptr<Expression>>> choices;
    std::vector<std::vector<SequentialStatement>> caseBodies;
};

struct FileDeclaration {
    std::vector<Name> names;
    Name type;
    Name mode;
    std::unique_ptr<Expression> path;
    SourceSpan span;
};

struct FunctionDeclaration {
    Name name;
    std::vector<GenericDeclaration> parameters;
    TypeMark returnType;
    std::vector<VariableDeclaration> variables;
    std::vector<ConstantDeclaration> constants;
    std::vector<SequentialStatement> statements;
    SourceSpan span;
    bool body = false;
    bool pure = true;
    bool procedure = false;
    std::vector<FileDeclaration> files;
};

struct LibraryClause {
    std::vector<Name> names;
    SourceSpan span;
};

struct UseClause {
    std::vector<Name> selectedName;
    SourceSpan span;
};

struct ContextClause {
    std::vector<LibraryClause> libraries;
    std::vector<UseClause> uses;
};

// Restricted positive-edge process syntax; names remain unresolved here.
enum class ClockEdgeForm { EventAndLevel, RisingEdgeCall, FallingEdgeCall };

struct ClockedProcess {
    Name sensitivity;
    Name eventSignal;
    Name levelSignal;
    std::string level;
    std::vector<Assignment> assignments;
    std::vector<VariableDeclaration> variables;
    std::optional<Name> enableSignal;
    std::string enableLevel;
    std::optional<Name> resetSignal;
    std::string resetLevel;
    std::vector<Assignment> resetAssignments;
    ClockEdgeForm edgeForm = ClockEdgeForm::EventAndLevel;
    SourceSpan span;
    // Structured bodies retain nested control flow until static elaboration.
    std::vector<SequentialStatement> statements;
    std::vector<Name> sensitivityList;
    std::vector<std::vector<Name>> sensitivityFields;
    bool combinational = false;
    bool allSensitivity = false;
    bool asynchronousReset = false;
    std::vector<SequentialStatement> resetStatements;
    std::optional<Name> label;
    std::vector<ConstantDeclaration> constants;
    std::unique_ptr<Expression> clockEnable;
    std::vector<ArrayTypeDeclaration> arrayTypes;
    std::vector<RecordTypeDeclaration> recordTypes;
    std::vector<EnumerationTypeDeclaration> enumerationTypes;
    std::vector<SubtypeDeclaration> subtypes;
    std::vector<FunctionDeclaration> functions;
};

struct EntityDeclaration {
    Name name;
    std::vector<GenericDeclaration> generics;
    std::vector<PortDeclaration> ports;
    SourceSpan span;
    ContextClause context;
};

struct GenericAssociation {
    std::optional<Name> formal;
    std::unique_ptr<Expression> actual;
    SourceSpan span;
};

struct EntityInstantiation {
    Name label;
    Name library;
    Name entity;
    std::optional<Name> architecture;
    std::vector<GenericAssociation> generics;
    std::vector<Name> actuals;
    SourceSpan span;
    std::vector<std::optional<Name>> formals;
    bool component = false;
    std::vector<std::vector<std::unique_ptr<Expression>>> actualIndices;
    std::vector<std::unique_ptr<Expression>> actualLiterals;
    std::vector<std::vector<std::unique_ptr<Expression>>> formalIndices;
    std::vector<bool> actualOpen;
};

struct GenerateStatement {
    Name iterator;
    DiscreteRange range;
    std::vector<Assignment> assignments;
    std::vector<GenerateStatement> generates;
    Name label;
    std::vector<EntityInstantiation> instantiations;
    bool conditional = false;
    std::unique_ptr<Expression> condition;
    // Alternatives are ordered elsif branches followed by an optional else.
    std::vector<GenerateStatement> alternatives;
    std::vector<ClockedProcess> processes;
    std::vector<SignalDeclaration> signals;
    std::vector<ConstantDeclaration> constants;
    std::vector<ArrayTypeDeclaration> arrayTypes;
    std::vector<RecordTypeDeclaration> recordTypes;
    std::vector<EnumerationTypeDeclaration> enumerationTypes;
    std::vector<SubtypeDeclaration> subtypes;
    std::vector<FunctionDeclaration> functions;
};

struct PackageDeclaration {
    Name name;
    ContextClause context;
    std::vector<ArrayTypeDeclaration> arrayTypes;
    std::vector<ConstantDeclaration> constants;
    std::vector<EntityDeclaration> components;
    bool body = false;
    std::vector<RecordTypeDeclaration> recordTypes;
    std::vector<EnumerationTypeDeclaration> enumerationTypes;
    std::vector<FunctionDeclaration> functions;
    std::vector<SubtypeDeclaration> subtypes;
};

struct ArchitectureBody {
    Name name;
    Name entity;
    std::vector<Assignment> assignments;
    std::vector<ClockedProcess> processes;
    std::vector<ArrayTypeDeclaration> arrayTypes;
    std::vector<SignalDeclaration> signals;
    std::vector<EntityInstantiation> instantiations;
    SourceSpan span;
    ContextClause context;
    std::vector<GenerateStatement> generates;
    std::vector<EntityDeclaration> components;
    std::vector<ConstantDeclaration> constants;
    std::vector<RecordTypeDeclaration> recordTypes;
    std::vector<EnumerationTypeDeclaration> enumerationTypes;
    std::vector<FunctionDeclaration> functions;
    std::vector<SubtypeDeclaration> subtypes;
    std::vector<AliasDeclaration> aliases;
};

struct DesignFile {
    std::vector<PackageDeclaration> packages;
    std::vector<EntityDeclaration> entities;
    std::vector<ArchitectureBody> architectures;
};

struct ParseDiagnostic {
    std::string message;
    SourceSpan span;
};

struct ParseWarning {
    std::string code;
    std::string message;
    SourceSpan span;
};

struct ParseResult {
    DesignFile syntax;
    std::vector<ParseDiagnostic> diagnostics;
    std::vector<ParseWarning> warnings;
    bool hasErrors() const { return !diagnostics.empty(); }
};

/// Parser for the initial RTL syntax slice. Unsupported constructs are diagnosed.
class Parser {
public:
    static ParseResult parse(std::string_view source, bool synthesis = false);
};

} // namespace vhdl
