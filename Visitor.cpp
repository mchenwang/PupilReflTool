#include "Visitor.h"

#include "llvm/IR/Constants.h"

#include <iostream>

using namespace clang;

namespace PReflTool {

namespace {
bool HasAnnotate(clang::Decl *decl, llvm::StringRef targetAnnotate) {
  if (decl->hasAttrs()) {
    auto attrs = decl->getAttrs();
    for (auto i = decl->specific_attr_begin<AnnotateAttr>(),
              end = decl->specific_attr_end<AnnotateAttr>();
         i != end; ++i) {
      auto attr = *i;

      if (attr->getAnnotation().equals(targetAnnotate))
        return true;
    }
  }
  return false;
}

int ConvertToBaseNumber(llvm::APInt integer) {
  SmallVector<char, 32> buffer;
  integer.toString(buffer, 10, true);
  buffer.push_back('\0');

  std::string numString(buffer.data());
  return std::stoi(numString);
}

double ConvertToBaseNumber(llvm::APFloat floating) {
  SmallVector<char, 32> buffer;
  floating.toString(buffer);
  buffer.push_back('\0');

  std::string numString(buffer.data());
  return std::stod(numString);
}

} // namespace

void Visitor::Visit(clang::Decl *decl) {
  bool flag = llvm::isa<NamespaceDecl>(decl) ||
              llvm::isa<CXXRecordDecl>(decl) ||
              llvm::isa<ClassTemplateDecl>(decl);

  if (flag) {
    m_cxxRecordFinder.TraverseDecl(decl);
  }
}

bool CXXRecordFinder::TraverseDecl(clang::Decl *decl) {
  if (!decl)
    return true;

  // As a syntax visitor, by default we want to ignore declarations for
  // implicit declarations (ones not typed explicitly by the user).
  if (!getDerived().shouldVisitImplicitCode() && decl->isImplicit()) {
    // Pupil Reflection Tool do not interesting in implicit template type
    // parameter. Just skip
    // if (auto *TTPD = dyn_cast<TemplateTypeParmDecl>(D))
    //  return TraverseTemplateTypeParamDeclConstraints(TTPD);
    return true;
  }

  switch (decl->getKind()) {
  case Decl::CXXRecord: {
    auto cxxRecordDecl = llvm::cast<CXXRecordDecl>(decl);
    if (!m_records.empty()) {
      // current CXXRecord is declared inside a class/struct
      m_namespaces.push_back(m_records.top()->GetName());
    }

    bool ifContinue = getDerived().TraverseCXXRecordDecl(cxxRecordDecl);

    if (m_records.top()->IsNeedGenerate())
      m_generator->PushCxxRecord(m_records.top());
    m_records.pop();

    if (!m_records.empty()) {
      m_namespaces.pop_back();
    }

    if (!ifContinue)
      return false;
    break;
  }
  case Decl::Namespace: {
    auto nspDecl = llvm::cast<NamespaceDecl>(decl);
    m_namespaces.push_back(nspDecl->getNameAsString());
    bool ifContinue = getDerived().TraverseNamespaceDecl(nspDecl);
    m_namespaces.pop_back();

    if (!ifContinue)
      return false;
    break;
  }
  case Decl::TemplateTypeParm: {
    auto tmpDecl = llvm::cast<TemplateTypeParmDecl>(decl);
    bool ifContinue = getDerived().TraverseTemplateTypeParmDecl(tmpDecl);

    if (!ifContinue)
      return false;
    break;
  }
#define ABSTRACT_DECL(DECL)
#define DECL(CLASS, BASE)                                                      \
  case Decl::CLASS: {                                                          \
    bool ifContinue =                                                          \
        getDerived().Traverse##CLASS##Decl(static_cast<CLASS##Decl *>(decl));  \
    m_templates.clear();                                                       \
    if (!ifContinue)                                                           \
      return false;                                                            \
    break;                                                                     \
  }
#include "PDeclNodes.inc"
  }
  return true;
}

bool CXXRecordFinder::VisitTemplateTypeParmDecl(
    clang::TemplateTypeParmDecl *decl) {
  m_templates.push_back(decl->getNameAsString());
  return true;
}

bool CXXRecordFinder::VisitCXXRecordDecl(clang::CXXRecordDecl *decl) {
  ECxxRecordType declType = ECxxRecordType::None;
  if (decl->isClass())
    declType = ECxxRecordType::Class;
  else if (decl->isStruct())
    declType = ECxxRecordType::Struct;

  auto record = std::make_unique<CxxRecord>(
      decl->getNameAsString(), m_namespaces, m_templates,
      HasAnnotate(decl, MetaAnnotate::name), declType);

  for (auto it = decl->bases_begin(); it != decl->bases_end(); ++it) {
    auto base = *it;
    if (base.getAccessSpecifier() == AS_public) {
      record->AddBase(base.getType().getAsString());
    }
  }

  m_records.push(std::move(record));
  // template parameters must reset for the next CXXRecord
  m_templates.clear();

  // deep into record to find fields
  m_cxxRecordVisitor.SetCxxRecord(m_records.top().get());
  m_cxxRecordVisitor.TraverseDecl(decl);
  return true;
}

bool CXXRecordVisitor::TraverseDecl(clang::Decl *decl) {
  if (!decl)
    return true;

  switch (decl->getKind()) {
  case Decl::Namespace:          // no namespace
  case Decl::TemplateTypeParm: { // skip template
    break;
  }
  case Decl::CXXRecord: // skip cxxRecord(skip fields which belong to internal
                        // classes)
  {
    if (!m_entered) {
      m_entered = true;
      bool ifContinue = getDerived().TraverseCXXRecordDecl(
          static_cast<CXXRecordDecl *>(decl));
      if (!ifContinue)
        return false;
    }
    break;
  }
#define ABSTRACT_DECL(DECL)
#define DECL(CLASS, BASE)                                                      \
  case Decl::CLASS: {                                                          \
    bool ifContinue =                                                          \
        getDerived().Traverse##CLASS##Decl(static_cast<CLASS##Decl *>(decl));  \
    if (!ifContinue)                                                           \
      return false;                                                            \
    break;                                                                     \
  }
#include "PDeclNodes.inc"
  }
  return true;
}

bool CXXRecordVisitor::VisitAccessSpecDecl(clang::AccessSpecDecl *decl) {
  auto access = decl->getAccess();
  EAccessPermission perm = EAccessPermission::Private;
  switch (access) {
  case clang::AS_public:
    perm = EAccessPermission::Public;
    break;
  case clang::AS_protected:
    perm = EAccessPermission::Protected;
    break;
  case clang::AS_private:
  case clang::AS_none:
  default:
    break;
  }
  m_record->SetCurrentAccessPermission(perm);
  return true;
}

bool CXXRecordVisitor::VisitFieldDecl(clang::FieldDecl *decl) {
  // only store parameters with meta annotation and in public permission
  if (HasAnnotate(decl, MetaAnnotate::name) &&
      m_record->IsCurrentFieldPublic()) {
    auto field = std::make_unique<Field>();
    field->name = decl->getNameAsString();

    m_attrVisitor.SetField(field.get());
    m_attrVisitor.TraverseFieldDecl(decl);

    m_record->PushField(field);
  }

  return true;
}

/* FIXME: need to filter VarDecl under Stmt
* void fun(){
*   int a; <- also is a VarDecl
* }
*/
bool CXXRecordVisitor::VisitVarDecl(clang::VarDecl *decl) {
  // same as field
  // to solve [constexpr] static members
  if (HasAnnotate(decl, MetaAnnotate::name) &&
      m_record->IsCurrentFieldPublic()) {
    auto field = std::make_unique<Field>();
    field->name = decl->getNameAsString();

    m_attrVisitor.SetField(field.get());
    m_attrVisitor.TraverseVarDecl(decl);

    m_record->PushField(field);
  }
  return true;
}

bool AttributeVisitor::VisitAnnotateAttr(clang::AnnotateAttr *an) {
  auto anName = an->getAnnotation();
  if (anName.equals(RangeAnnotate::name)) {
    auto pRange = std::make_unique<RangeAnnotate>();
    m_annotateValueGetter.SetAttr(pRange.get());
    m_annotateValueGetter.TraverseAnnotateAttr(an);

    m_field->attrs.emplace_back(std::move(pRange));
  } else if (anName.equals(InfoAnnotate::name)) {
    auto pInfo = std::make_unique<InfoAnnotate>();
    m_annotateValueGetter.SetAttr(pInfo.get());
    m_annotateValueGetter.TraverseAnnotateAttr(an);

    m_field->attrs.emplace_back(std::move(pInfo));
  } else if (anName.equals(StepAnnotate::name)) {
    auto pStep = std::make_unique<StepAnnotate>();
    m_annotateValueGetter.SetAttr(pStep.get());
    m_annotateValueGetter.TraverseAnnotateAttr(an);

    m_field->attrs.emplace_back(std::move(pStep));
  }
  return true;
}

//bool RangeAnnotateVisitor::VisitConstantExpr(clang::ConstantExpr *ce) {
//  auto t = ce->getType();
//  if (t.getAsString() == "double" || t.getAsString() == "float") {
//    SmallVector<char, 32> buffer;
//    ce->getAPValueResult().getFloat().toString(buffer);
//    buffer.push_back('\0');
//
//    std::string numString(buffer.data());
//    double num = std::stod(numString);
//    m_rangeAnnotate->SetRange(num);
//  } else {
//    SmallVector<char, 32> buffer;
//    ce->getAPValueResult().getInt().toString(buffer);
//    buffer.push_back('\0');
//
//    std::string numString(buffer.data());
//    int num = std::stoi(numString);
//    m_rangeAnnotate->SetRange(num);
//  }
//  return true;
//}

bool AnnotateValueGetter::VisitStringLiteral(clang::StringLiteral *str) {
  if (!m_attr)
    return true;

  if (m_attr->GetName().compare(InfoAnnotate::name) == 0) {
    auto *attr = static_cast<InfoAnnotate *>(m_attr);
    attr->info = str->getString().str();
  }
  return true;
}

bool AnnotateValueGetter::VisitIntegerLiteral(clang::IntegerLiteral *integer) {
  if (!m_attr)
    return true;

  if (m_attr->GetName().compare(RangeAnnotate::name) == 0) {
    auto *attr = static_cast<RangeAnnotate *>(m_attr);
    attr->SetRange(ConvertToBaseNumber(integer->getValue()));
  } else if (m_attr->GetName().compare(StepAnnotate::name) == 0) {
    auto *attr = static_cast<StepAnnotate *>(m_attr);
    attr->step = ConvertToBaseNumber(integer->getValue());
  }
  return true;
}

bool AnnotateValueGetter::VisitFloatingLiteral(clang::FloatingLiteral *floating) {
  if (!m_attr)
    return true;

  if (m_attr->GetName().compare(RangeAnnotate::name) == 0) {
    auto *attr = static_cast<RangeAnnotate *>(m_attr);
    attr->SetRange(ConvertToBaseNumber(floating->getValue()));
  } else if (m_attr->GetName().compare(StepAnnotate::name) == 0) {
    auto *attr = static_cast<StepAnnotate *>(m_attr);
    attr->step = ConvertToBaseNumber(floating->getValue());
  }
  return true;
}

} // namespace PReflTool