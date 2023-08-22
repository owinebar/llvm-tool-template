//===---- tools/extra/ToolTemplate.cpp - Template for refactoring tool ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements an empty refactoring tool using the clang tooling.
//  The goal is to lower the "barrier to entry" for writing refactoring tools.
//
//  Usage:
//  tool-template <cmake-output-dir> <file1> <file2> ...
//
//  Where <cmake-output-dir> is a CMake build directory in which a file named
//  compile_commands.json exists (enable -DCMAKE_EXPORT_COMPILE_COMMANDS in
//  CMake to get this output).
//
//  <file1> ... specify the paths of files in the CMake source tree. This path
//  is looked up in the compile command database. If the path of a file is
//  absolute, it needs to point into CMake's source tree. If the path is
//  relative, the current working directory needs to be in the CMake source
//  tree and the file must be in a subdirectory of the current working
//  directory. "./" prefixes in the relative files will be automatically
//  removed, but the rest of a relative path must be a suffix of a path in
//  the compile command line database.
//
//  For example, to use tool-template on all files in a subtree of the
//  source tree, use:
//
//    /path/in/subtree $ find . -name '*.cpp'|
//        xargs tool-template /path/to/build
//
//===----------------------------------------------------------------------===//

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Execution.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Refactoring/AtomicChange.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Signals.h"
#include "ResourcePath.h"
#include <memory>
#include <ostream>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

namespace {

class ConverterCallback : public MatchFinder::MatchCallback {
public:
  ConverterCallback(ExecutionContext *Context) : Context(Context) {}

  void run(const MatchFinder::MatchResult &Result) override {
    // TODO: This routine will get called for each thing that the matchers
    // find.
    // At this point, you can examine the match, and do whatever you want,
    // including replacing the matched text with other text
    auto *D = Result.Nodes.getNodeAs<NamedDecl>("decl");
    assert(D);
    // Use AtomicChange to get a key.
    if (D->getBeginLoc().isValid()) {
      AtomicChange Change(*Result.SourceManager, D->getBeginLoc());
      Context->reportResult(Change.getKey(), D->getQualifiedNameAsString());
    }
  }

  void onStartOfTranslationUnit() override {
    Context->reportResult("START", "Start of TU.");
  }
  void onEndOfTranslationUnit() override {
    Context->reportResult("END", "End of TU.");
  }

private:
  ExecutionContext *Context;
};
} // end anonymous namespace

// Set up the command line options
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::OptionCategory cppgcConvertCategory("cpp-gc options");



int main(int argc, const char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

  AbsorbOptionsIntoCategory(cppgcConvertCategory,&ExecutorCategory);

  auto ExpectedExecutor = clang::tooling::createExecutorFromCommandLineArgs(
      argc, argv, cppgcConvertCategory);

  if (!ExpectedExecutor) {
    llvm::errs() << llvm::toString(ExpectedExecutor.takeError()) << "\n";
    return 1;
  }
  std::unique_ptr<ToolExecutor> &Executor = ExpectedExecutor.get();

  ExecutionContext *Context =  Executor->getExecutionContext();
  ast_matchers::MatchFinder Finder;
  ConverterCallback Callback(Context);

  // TODO: Put your matchers here.
  // Use Finder.addMatcher(...) to define the patterns in the AST that you
  // want to match against. You are not limited to just one matcher!
  //
  // This is a sample matcher:
  Finder.addMatcher(
      namedDecl(cxxRecordDecl(), isExpansionInMainFile()).bind("decl"),
      &Callback);
  ArgumentsAdjuster handleResourceDir;
  if (UseCDBResources.getValue()) {
    handleResourceDir = getResourceDirFromCDBAdjuster();
  } else {
    handleResourceDir = getResourceDirFromTool(ToolResourceDir.getValue());
  }
  auto Err = Executor->execute(newFrontendActionFactory(&Finder),handleResourceDir);
  if (Err) {
    llvm::errs() << llvm::toString(std::move(Err)) << "\n";
  }
  Executor->getToolResults()->forEachResult(
      [](llvm::StringRef key, llvm::StringRef value) {
        llvm::errs() << "----" << key.str() << "\n" << value.str() << "\n";
      });
}
