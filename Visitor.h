#pragma once

#include "clang/AST/RecursiveASTVisitor.h"

#include "Generator.h"

#include <stack>
#include <memory>

namespace PReflTool {

class AnnotateValueGetter
    : public clang::RecursiveASTVisitor<AnnotateValueGetter> {
  Attr *m_attr;

public:
  AnnotateValueGetter() : m_attr(nullptr) {}
  void SetAttr(Attr *attr) { m_attr = attr; }

  bool VisitStringLiteral(clang::StringLiteral *str);
  bool VisitIntegerLiteral(clang::IntegerLiteral *integer);
  bool VisitFloatingLiteral(clang::FloatingLiteral *floating);
};

// used to get the parameters of annotate::range(min, max)
//class RangeAnnotateVisitor
//    : public clang::RecursiveASTVisitor<RangeAnnotateVisitor> {
//  RangeAnnotate *m_rangeAnnotate;
//
//public:
//  RangeAnnotateVisitor() : m_rangeAnnotate(nullptr) {}
//  void SetRangeAnnotate(RangeAnnotate *rangeAnnotate) {
//    m_rangeAnnotate = rangeAnnotate;
//  }
//  bool VisitConstantExpr(clang::ConstantExpr *ce);
//};

class AttributeVisitor : public clang::RecursiveASTVisitor<AttributeVisitor> {
  AnnotateValueGetter m_annotateValueGetter;
  Field *m_field;

public:
  AttributeVisitor() : m_field(nullptr) {}
  void SetField(Field *field) { m_field = field; }
  bool VisitAnnotateAttr(clang::AnnotateAttr *an);
};

// used to traverse cxx record to get the information of fields/inheritance
class CXXRecordVisitor : public clang::RecursiveASTVisitor<CXXRecordVisitor> {
  AttributeVisitor m_attrVisitor;

  CxxRecord *m_record;

  bool m_entered;

public:
  CXXRecordVisitor(CxxRecord *record = nullptr)
      : m_record(record), m_attrVisitor(), m_entered(false) {}

  void SetCxxRecord(CxxRecord *record) {
    m_record = record;
    m_entered = false;
  }

  bool VisitFieldDecl(clang::FieldDecl *decl);
  bool VisitVarDecl(clang::VarDecl *decl);
  bool VisitAccessSpecDecl(clang::AccessSpecDecl *decl);
  bool TraverseDecl(clang::Decl *decl);
};

// used to find cxx records (with namespaces and templates)
class CXXRecordFinder : public clang::RecursiveASTVisitor<CXXRecordFinder> {
  CXXRecordVisitor m_cxxRecordVisitor;
  PReflTool::Generator *m_generator;
  std::vector<std::string> m_namespaces;
  std::vector<std::string> m_templates;

  std::stack<std::unique_ptr<CxxRecord>> m_records;

public:
  CXXRecordFinder(PReflTool::Generator *g) : m_generator(g) {}
  bool VisitCXXRecordDecl(clang::CXXRecordDecl *decl);
  bool VisitTemplateTypeParmDecl(clang::TemplateTypeParmDecl *decl);
  bool TraverseDecl(clang::Decl *decl);
};

class Visitor {
  CXXRecordFinder m_cxxRecordFinder;
  clang::SourceManager &m_sm;

public:
  Visitor(clang::SourceManager &sm, PReflTool::Generator *g)
      : m_sm(sm), m_cxxRecordFinder(g) {}

  clang::SourceManager &GetSourceManager() const { return m_sm; }

  void Visit(clang::Decl *decl);
};
} // namespace PReflTool