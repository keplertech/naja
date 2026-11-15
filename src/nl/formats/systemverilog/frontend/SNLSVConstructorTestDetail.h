// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "NLID.h"

namespace slang::ast {
class Symbol;
}

namespace naja::NL {
class SNLInstance;
class SNLDesign;
}

namespace naja::NL::detail {

struct InferredMemoryGuardDefaults {
    int kind {0};
    bool polarity {false};
    bool exprNull {false};
    bool caseExprNull {false};
    bool caseStmtNull {false};
    bool caseItemNull {false};
    bool hasSourceRange {false};
};

struct FindTimedStatementTestResult {
    bool foundTimed {false};
    bool reportedUnsupported {false};
    std::string unsupportedReason;
};

struct CollectDirectAssignmentsTestResult {
    bool success {false};
    size_t assignmentCount {0};
    std::string failureReason;
};

struct ForLoopStepExpressionTestResult {
    bool success {false};
    int64_t loopValue {0};
    std::string failureReason;
};

struct ResolveAssignmentLHSBitsTestResult {
    bool success {false};
    size_t bitCount {0};
    std::string failureReason;
};

struct SingleLHSFallbackPathMaxTestResult {
    bool success {false};
    size_t maxAssignments {0};
    std::string failureReason;
};

struct ProceduralReplayEnvMergeTestResult {
    bool success {false};
    size_t mergedSymbolCount {0};
    bool missingTrueSymbolCopied {false};
    bool externalSymbolOverrodeBranches {false};
    bool widthMismatchRejected {false};
    std::string failureReason;
    std::string widthMismatchFailureReason;
};

struct ActiveForLoopConstantHelpersTestResult {
    bool emptyBindingsRejected {false};
    bool symbolOnlyVariableHit {false};
    bool symbolDescriptionHit {false};
    bool nameDescriptionHit {false};
    bool crossScopeSymbolNameHit {false};
    bool emptyIdentifierRejected {false};
    bool missingSourceRejected {false};
    bool nameSourceHit {false};
    bool negativeUnsignedRejected {false};
    bool parameterUnsignedResolved {false};
    bool parameterInt64Resolved {false};
    bool multiplySourceOverflowRejected {false};
    bool negativeEqualityOperandRejected {false};
    bool unknownParameterInt64Rejected {false};
    bool conditionalLoopConditionHit {false};
    bool concatLoopOperandHit {false};
    bool callLoopArgHit {false};
    bool constantConcatNoRuntimeSymbol {false};
    bool systemQueryCallNoRuntimeSymbol {false};
    bool callRuntimeArgHit {false};
};

struct SourceExcerptTestOptions {
    std::string sourceText;
    std::optional<size_t> startOffset {0};
    std::optional<size_t> endOffset {1};
    size_t maxLength {160};
    bool preserveRawBufferSize {false};
    bool useAlternateEndBuffer {false};
};

std::optional<std::string> testSVConstructorGetSourceExcerpt(
  const SourceExcerptTestOptions& options);

std::string testSVConstructorFormatReasonWithSourceExcerptNoRange(
  const std::string& reason);

bool testSVConstructorTryPowerInt64(
  int64_t base,
  int64_t exponent,
  int64_t& value);

InferredMemoryGuardDefaults testSVConstructorDefaultInferredMemoryGuard();

std::optional<std::vector<int32_t>> testSVConstructorCollectIndexedRangeElementIndices(
  int32_t startIndex,
  int32_t sliceWidth,
  bool indexedDown);

std::optional<size_t> testSVConstructorResolveFixedUnpackedArraySelectionBitCountFromAssignRhs(
  const std::string& sourceText);

std::vector<bool> testSVConstructorEncodeUnsignedProductBits(
  uint64_t leftValue,
  uint64_t rightValue,
  size_t targetWidth);

std::optional<std::string> testSVConstructorCreatePowerOfTwoBitsFromAssignRhs(
  const std::string& sourceText,
  size_t targetWidth);

std::optional<size_t> testSVConstructorResolveWildcardPatternWidthFallback(
  const std::optional<size_t>& directWidth,
  bool canonicalIntegral,
  int32_t bitWidth);

std::optional<std::string> testSVConstructorResolveWildcardCaseItemPatternFromAssignRhs(
  const std::string& sourceText,
  size_t targetWidth,
  bool signExtend);

std::optional<std::string> testSVConstructorResolveUnknownLiteralBitsAsZeroFromAssignRhs(
  const std::string& sourceText,
  size_t targetWidth);

std::optional<std::string> testSVConstructorResolveExpressionBitsFromAssignRhs(
  const std::string& sourceText,
  size_t targetWidth);

std::optional<size_t> testSVConstructorResolveAssignmentLHSBitsFromAssignLhs(
  const std::string& sourceText,
  bool allowConcatenation = false);

std::optional<ResolveAssignmentLHSBitsTestResult>
testSVConstructorResolveAssignmentLHSBitsResultFromAssignLhs(
  const std::string& sourceText,
  bool allowConcatenation = false);

std::optional<ResolveAssignmentLHSBitsTestResult>
testSVConstructorResolveAssignmentLHSBitsReportedFailureFromAssignLhs(
  const std::string& sourceText,
  bool allowConcatenation = false);

std::optional<std::string> testSVConstructorResolveConstantExpressionBitsFromAssignRhs(
  const std::string& sourceText,
  size_t targetWidth);

std::vector<bool> testSVConstructorEncodeSignedInt64ConstantBits(
  int64_t value,
  size_t targetWidth);

std::optional<FindTimedStatementTestResult> testSVConstructorFindTimedStatementFromProceduralBlock(
  const std::string& sourceText);

std::optional<CollectDirectAssignmentsTestResult>
testSVConstructorCollectDirectAssignmentsFromProceduralBlock(
  const std::string& sourceText);

std::optional<SingleLHSFallbackPathMaxTestResult>
testSVConstructorGetSingleLHSFallbackPathAssignmentMaxFromProceduralBlock(
  const std::string& sourceText);

std::optional<ProceduralReplayEnvMergeTestResult>
testSVConstructorMergeProceduralReplayEnvs();

bool testSVConstructorAutomaticLocalReplayLHSReuseEligibility(
  bool wholeLHS,
  bool automaticLocal,
  bool hasReplayBits);

std::optional<ActiveForLoopConstantHelpersTestResult>
testSVConstructorActiveForLoopConstantHelpers();

std::optional<ForLoopStepExpressionTestResult>
testSVConstructorApplyForLoopStepExpressionFromForLoop(
  const std::string& sourceText,
  int64_t initialLoopValue);

std::optional<ForLoopStepExpressionTestResult>
testSVConstructorApplyForLoopStepExpressionFromProceduralStatement(
  const std::string& sourceText,
  int64_t initialLoopValue);

std::optional<ForLoopStepExpressionTestResult>
testSVConstructorApplyConstantCompoundForLoopOperator(
  const std::string& opText,
  int64_t initialLoopValue,
  int64_t rhsValue);

std::optional<bool> testSVConstructorConvertConstantToIntegerFromAssignRhs(
  const std::string& sourceText);

std::optional<std::string> testSVConstructorAppendSignedConstantBits(
  int64_t value,
  size_t targetWidth);

std::optional<bool> testSVConstructorSameExpressionStructureFromContinuousAssignRhsPair(
  const std::string& sourceText);

std::string testSVConstructorFormatAssignmentLHSResolutionFailureReason(
  bool hasLhsNet,
  const std::string& lhsDescription,
  const std::string& failureReason);

std::string testSVConstructorFormatSequentialConcatLeafFailureReason(
  const std::string& lhsDescription,
  const std::string& leafFailureReason);

std::string testSVConstructorFormatDescribedFailure(
  const std::string& prefix,
  const std::string& description);

std::string testSVConstructorFormatQuotedDescriptionFailure(
  const std::string& prefix,
  const std::string& description);

bool testSVConstructorCheckCombinationalBlockCycles(
  SNLDesign* design, NLID::DesignObjectID firstInstanceID, std::string& reason);

bool testSVConstructorTryCollapseAnonymousAssignAlias(
  SNLInstance* assignInstance,
  const slang::ast::Symbol* associatedSymbol = nullptr);

}  // namespace naja::NL::detail
