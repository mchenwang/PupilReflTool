#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#include <iostream>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "Generator.h"
#include "Visitor.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory s_toolingCategory("Tooling Sample");

class Analyzer : public clang::ASTConsumer {
  PReflTool::Visitor m_visitor;

public:
  Analyzer(clang::SourceManager &sm, PReflTool::Generator *g)
      : m_visitor(sm, g) {}

  void HandleTranslationUnit(clang::ASTContext &context) final {
    auto decls = context.getTranslationUnitDecl()->decls();
    auto &sm = m_visitor.GetSourceManager();

    for (auto &decl : decls) {
      const auto &fileID = sm.getFileID(decl->getLocation());
      // Filter out decls in the include files.
      if (fileID != sm.getMainFileID())
        continue;

      m_visitor.Visit(decl);
    }
  }
};

class AnalyzerAction : public clang::ASTFrontendAction {
  PReflTool::Generator *m_generator;

public:
  AnalyzerAction(PReflTool::Generator *g) : m_generator(g) {}

  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &ci, clang::StringRef) final {
    ci.getDiagnostics().setClient(new IgnoringDiagConsumer());
    return std::unique_ptr<clang::ASTConsumer>(
        new Analyzer(ci.getSourceManager(), m_generator));
  }
};

std::unique_ptr<FrontendActionFactory>
NewAnalyzerActionFactory(PReflTool::Generator *g) {
  class AnalyzerActionFactory : public FrontendActionFactory {
    PReflTool::Generator *m_generator;

  public:
    AnalyzerActionFactory(PReflTool::Generator *g) : m_generator(g) {}

    std::unique_ptr<FrontendAction> create() override {
      return std::make_unique<AnalyzerAction>(m_generator);
    }
  };

  return std::unique_ptr<FrontendActionFactory>(new AnalyzerActionFactory(g));
}

void RunTool(std::string file) {
  PReflTool::Generator generator{file};
  if (generator.CheckModifyTime()) {
    std::cout << generator.GetGeneratedFilePath().stem()
              << "'s reflection file does not need to be regenerated.\n";
    return;
  }
  const char *args[] = {
      "",
      file.data(),
      "--",
      "-xc++",
      "-D",
      PReflTool::MetaAnnotate::GetMarco(),
      "-D",
      PReflTool::RangeAnnotate::GetMarco(),
      "-D",
      PReflTool::InfoAnnotate::GetMarco(),
      "-D",
      PReflTool::StepAnnotate::GetMarco(),
      "INFO(str)=clang::annotate(\"info\", str)",
      "-std=c++20",                     // use c++ 20
      "-Wno-pragma-once-outside-header" // ignore #pragma once warning
  };
  int argc = _countof(args);
  auto expectedParser =
      CommonOptionsParser::create(argc, args, s_toolingCategory);

  if (!expectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << expectedParser.takeError();
    return;
  }

  CommonOptionsParser &optionsParser = expectedParser.get();
  ClangTool tool(optionsParser.getCompilations(),
                 optionsParser.getSourcePathList());

  tool.run(NewAnalyzerActionFactory(&generator).get());
  generator.Generate();
}

const std::filesystem::path TEST_DIR = CMAKE_DEF_PREFLTOOL_DEFAULT;

int main(int argc, char** args) {
  std::vector<std::string> files;

  if (argc > 1) {
    files.reserve(static_cast<size_t>(argc) - 1);
    for (int i = 1; i < argc; i++)
      files.push_back(std::string{args[i]});
  } else {
#if defined(DEBUG) || defined(_DEBUG)
    files.resize(1);
    files[0] = (TEST_DIR / "test/test.h").string();
#else
    std::cout << "Please input target files.\n";
    return 0;
#endif // DEBUG
  }

  for (auto &fileName : files) {
    std::filesystem::path filePath{fileName};
    if (std::filesystem::exists(filePath)) {
      filePath.make_preferred();
      std::cout << "*** start file: " << filePath.string().data() << "\n";
      RunTool(filePath.string());
    } else {
      std::cerr << "*** error : " << fileName << " does not exist\n";
    }
  }
  return 0;
}
