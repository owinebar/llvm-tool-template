
#include <llvm/Support/CommandLine.h>
#include <string>
#include <llvm/Support/Error.h>
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/StandaloneExecution.h"
#include "clang/Tooling/ArgumentsAdjusters.h"

namespace clang {
namespace tooling {

extern llvm::cl::opt<bool> UseCDBResources;
extern llvm::cl::opt<std::string> ToolResourceDir;
extern llvm::cl::OptionCategory ExecutorCategory;

inline llvm::Error make_string_error(const llvm::Twine &Message) {
  return llvm::make_error<llvm::StringError>(Message,
                                             llvm::inconvertibleErrorCode());
}
std::string getClangPath();
llvm::Expected<std::unique_ptr<StandaloneToolExecutor>>
createStandaloneExecutorFromCommandLineArgs(int &argc, const char **argv,
                                      llvm::cl::OptionCategory &Category,
                                      const char *Overview = nullptr);

void AddOptionsToCategory(llvm::cl::OptionCategory &Category,
                          llvm::cl::SubCommand &Sub = llvm::cl::SubCommand::getTopLevel(),
                          llvm::cl::OptionHidden Visibility = llvm::cl::NotHidden);

void AddOptionsToCategory(llvm::cl::OptionCategory &Category,
                          llvm::cl::OptionCategory *From = nullptr,
                          llvm::cl::SubCommand &Sub = llvm::cl::SubCommand::getTopLevel(),
                          llvm::cl::OptionHidden Visibility = llvm::cl::NotHidden);

void AddOptionsToCategory(llvm::cl::OptionCategory &Category,
                          ArrayRef<const llvm::cl::OptionCategory *> From,
                          llvm::cl::SubCommand &Sub = llvm::cl::SubCommand::getTopLevel(),
                          llvm::cl::OptionHidden Visibility = llvm::cl::NotHidden);

void AbsorbOptionsIntoCategory(llvm::cl::OptionCategory &Category,
                               llvm::cl::SubCommand &Sub = llvm::cl::SubCommand::getTopLevel(),
                               llvm::cl::OptionHidden Visibility = llvm::cl::NotHidden);

void AbsorbOptionsIntoCategory(llvm::cl::OptionCategory &Category,
                               llvm::cl::OptionCategory *From = nullptr,
                               llvm::cl::SubCommand &Sub = llvm::cl::SubCommand::getTopLevel(),
                               llvm::cl::OptionHidden Visibility = llvm::cl::NotHidden);

void AbsorbOptionsIntoCategory(llvm::cl::OptionCategory &Category,
                               ArrayRef<const llvm::cl::OptionCategory *> From,
                               llvm::cl::SubCommand &Sub = llvm::cl::SubCommand::getTopLevel(),
                               llvm::cl::OptionHidden Visibility = llvm::cl::NotHidden);

ArgumentsAdjuster getResourceDirFromCDBAdjuster();
ArgumentsAdjuster getResourceDirFromTool(std::string &resdir);
}
}
