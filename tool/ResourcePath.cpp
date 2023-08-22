
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/FileSystem.h"
#include <llvm/Support/CommandLine.h>
#include <memory>
#include <string>
#include <unistd.h>
#include <dlfcn.h>
#include <limits.h>
#include <utility>
#include "clang/Tooling/Execution.h"
#include "clang/Tooling/StandaloneExecution.h"
#include "clang/Tooling/ToolExecutorPluginRegistry.h"
#include "clang/Tooling/Tooling.h"
#include "ResourcePath.h"
#include "clang/Tooling/ArgumentsAdjusters.h"
#include "clang/Driver/Driver.h"

#if !defined(PATH_MAX)
a// For GNU Hurd
#if defined(__GNU__)
#define PATH_MAX 4096
#elif defined(__MVS__)
#define PATH_MAX _XOPEN_PATH_MAX
#endif
#endif


using namespace llvm;

namespace clang {
namespace tooling {
cl::OptionCategory ExecutorCategory("Tool Options");

void AddOptionsToCategory(cl::OptionCategory &Category, cl::SubCommand &Sub,cl::OptionHidden Visibility) {  
  for (auto &I : Sub.OptionsMap) {
    bool Unrelated = true;
    for (auto &Cat : I.second->Categories) {
      if (Cat == &Category )
        Unrelated = false;
    }
    if (Unrelated) {
      I.second->addCategory(Category);
      I.second->setHiddenFlag(Visibility);
    }
  }
}

void AddOptionsToCategory(cl::OptionCategory &Category,cl::OptionCategory *From, cl::SubCommand &Sub,cl::OptionHidden Visibility) {  
  if (!From) {
    AddOptionsToCategory(Category,Sub,Visibility);
    return;
  }
  for (auto &I : Sub.OptionsMap) {
    bool Unrelated = false;
    for (auto &Cat : I.second->Categories) {
      if (Cat == From )
        Unrelated = true;
    }
    if (Unrelated) {
      I.second->addCategory(Category);
      I.second->setHiddenFlag(Visibility);
    }
  }
}


void AddOptionsToCategory(cl::OptionCategory &Category,ArrayRef<const cl::OptionCategory *> From,
                               cl::SubCommand &Sub,cl::OptionHidden Visibility) {
  for (auto &I : Sub.OptionsMap) {
    bool Unrelated = false;
    for (auto &Cat : I.second->Categories) {
      if (is_contained(From, Cat))
        Unrelated = true;
    }
    if (Unrelated) {
      I.second->addCategory(Category);
      I.second->setHiddenFlag(Visibility);
    }
  }
}

void AbsorbOptionsIntoCategory(cl::OptionCategory &Category, cl::SubCommand &Sub,cl::OptionHidden Visibility) {  
  for (auto &I : Sub.OptionsMap) {
    bool Unrelated = true;
    for (auto &Cat : I.second->Categories) {
      if (Cat == &Category )
        Unrelated = false;
    }
    if (Unrelated) {
      I.second->Categories[0] = &Category;
      I.second->setHiddenFlag(Visibility);
    }
  }

}

void AbsorbOptionsIntoCategory(cl::OptionCategory &Category,cl::OptionCategory *From, cl::SubCommand &Sub,cl::OptionHidden Visibility) {  
  if (!From) {
    AbsorbOptionsIntoCategory(Category,Sub,Visibility);
    return;
  }
  for (auto &I : Sub.OptionsMap) {
    bool Unrelated = false;
    for (auto &Cat : I.second->Categories) {
      if (Cat == From )
        Unrelated = true;
    }
    if (Unrelated) {
      I.second->Categories[0] = &Category;
      I.second->setHiddenFlag(Visibility);
    }
  }
}

void AbsorbOptionsIntoCategory(cl::OptionCategory &Category,ArrayRef<const cl::OptionCategory *> From,
                               cl::SubCommand &Sub,cl::OptionHidden Visibility) {
  for (auto &I : Sub.OptionsMap) {
    bool Unrelated = false;
    for (auto &Cat : I.second->Categories) {
      if (is_contained(From, Cat))
        Unrelated = true;
    }
    if (Unrelated) {
      I.second->Categories[0] = &Category;
      // I.second->addCategory(Category);
      I.second->setHiddenFlag(Visibility);
    }
  }
}

llvm::cl::opt<std::string>
  ToolResourceDir("tool-resource-dir", llvm::cl::desc("The location of the resource directory for the tool."),
                  cl::Optional,
                  cl::ValueRequired,
                  cl::NotHidden,
                  cl::cat(ExecutorCategory),
                  llvm::cl::init(clang::driver::Driver::GetResourcesPath(getClangPath())));
llvm::cl::opt<bool>
  UseCDBResources("use-compdb-resources", llvm::cl::desc("Set the compiler resource directory from the compilation database entry for each file."),
                  cl::Optional,
                  cl::ValueRequired,
                  cl::NotHidden,
                  cl::cat(ExecutorCategory),
                  llvm::cl::init(false));

std::string getClangPath() {
    Dl_info libclang_info{};
    void *libclang = dlopen("libclang.so",RTLD_LAZY);
    if (libclang) {
        // use a symbol defined since the inception of the library
        void *spcl = dlsym(libclang,"clang_createTranslationUnit");
        int check = dladdr(spcl,&libclang_info);
        dlclose(libclang);
        if (check && libclang_info.dli_fname) {
            char real_path[PATH_MAX];
            if (realpath(libclang_info.dli_fname, real_path))
                return std::string(real_path);
        }
    }
    // if statically linked, try whatever this function is part of
    int check = dladdr((void *) &getClangPath,&libclang_info);
    if (check && libclang_info.dli_fname) {
        char real_path[PATH_MAX];
        if (realpath(libclang_info.dli_fname, real_path))
            return std::string(real_path);
    }
    return std::string("");
}

llvm::Expected<std::unique_ptr<StandaloneToolExecutor>>
createStandaloneExecutorFromCommandLineArgs(int &argc, const char **argv,
                                      llvm::cl::OptionCategory &Category,
                                      const char *Overview) {
  auto ExpectedOptionsParser =
      CommonOptionsParser::create(argc, argv, Category, llvm::cl::ZeroOrMore,
                                  /*Overview=*/Overview);
  if (!ExpectedOptionsParser)
    return ExpectedOptionsParser.takeError();
  CommonOptionsParser &optP { ExpectedOptionsParser.get() };
  if (optP.getSourcePathList().empty())
    return make_string_error("No positional argument found.");
  

  return std::move(std::make_unique<StandaloneToolExecutor>(std::move(optP)));
}

/// Set resource dir according to command from CompilationDatabase
ArgumentsAdjuster getResourceDirFromCDBAdjuster() {
  return [](const CommandLineArguments &Args, StringRef /*unused*/) {    
    CommandLineArguments AdjustedArgs;
    bool HasResourceDir = false;
    for (size_t i = 0, e = Args.size(); i < e; ++i) {
      StringRef Arg = Args[i];
      if (Arg.starts_with("-resource-dir")) {
        HasResourceDir = true;
      }
      AdjustedArgs.push_back(Args[i]);
    }
    if (!HasResourceDir) {
      char real_path[PATH_MAX];
      std::string binpath{realpath(Args[0].c_str(), real_path)};
      AdjustedArgs.push_back("-resource-dir=" + 
                             clang::driver::Driver::GetResourcesPath(binpath));
    }
    return AdjustedArgs;
  };
}
ArgumentsAdjuster getResourceDirFromTool(std::string &resdir) {
  return [resdir](const CommandLineArguments &Args, StringRef /*unused*/) {    
    CommandLineArguments AdjustedArgs;
    for (size_t i = 0, e = Args.size(); i < e; ++i) {
      StringRef Arg = Args[i];
      // ignore user specified resource directory from compilation database
      if (!Arg.starts_with("-resource-dir")) {
        AdjustedArgs.push_back(Args[i]);
      }
    }
    AdjustedArgs.push_back("-resource-dir=" + resdir);
    return AdjustedArgs;
  };
}

}
}

