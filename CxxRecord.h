#pragma once

#include "Attributes.h"

#include <vector>
#include <memory>

namespace PReflTool {

struct Field {
  std::string name;
  std::vector<std::unique_ptr<Attr>> attrs;
};

enum class ECxxRecordType {
  Class,
  Struct,
  None // union not implemented
};

enum class EAccessPermission {
    Public,
    Private,
    Protected
};

class CxxRecord {
  std::string m_name;
  std::vector<std::string> m_namespaces;
  std::vector<std::string> m_templates;
  std::vector<std::string> m_bases;
  bool m_hasMetaFlag;

  std::vector<std::unique_ptr<Field>> m_fields;

  const ECxxRecordType m_type;
  EAccessPermission m_curFlag;

public:
  CxxRecord(std::string name, std::vector<std::string> &nsps,
            std::vector<std::string> &tmps, bool hasMetaFlag,
            ECxxRecordType type)
      : m_name(name), m_namespaces(nsps), m_templates(tmps),
        m_hasMetaFlag(hasMetaFlag), m_type(type) {
    switch (m_type) {
    case ECxxRecordType::Struct:
      m_curFlag = EAccessPermission::Public;
      break;
    case ECxxRecordType::Class:
      m_curFlag = EAccessPermission::Private;
      break;
    case ECxxRecordType::None:
    default:
      m_curFlag = EAccessPermission::Private;
      break;
    }
  }

  std::string GetName() const { return m_name; }

  const std::vector<std::string> &GetNamespaces() const { return m_namespaces; }
  const std::vector<std::string> &GetTemplates() const { return m_templates; }
  const std::vector<std::string> &GetBases() const { return m_bases; }
  const std::vector<std::unique_ptr<Field>> &GetFields() const {
    return m_fields;
  }

  bool IsNeedGenerate() const { return m_hasMetaFlag; }

  void PushField(std::unique_ptr<Field> &field) {
    m_fields.emplace_back(std::move(field));
  }

  void AddBase(std::string base) { m_bases.push_back(base); }

  void SetCurrentAccessPermission(EAccessPermission permission) {
    m_curFlag = permission;
  }

  bool IsCurrentFieldPublic() const {
    return m_curFlag == EAccessPermission::Public;
  }
};

} // namespace PReflTool