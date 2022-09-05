#pragma once

#include "CxxRecord.h"
#include <filesystem>

namespace PReflTool {
class Generator {
private:
  std::filesystem::path m_targetFile;
  std::filesystem::path m_resultDir;
  std::vector<std::unique_ptr<CxxRecord>> m_records;

  void AddIncludePathToTarget();

public:
  Generator(std::string file);
  ~Generator() = default;
	
  void PushCxxRecord(std::unique_ptr<CxxRecord> &record) {
    m_records.emplace_back(std::move(record));
  }

  // If generated file exsits, compare the modification time of generated file
  // and traget file. When the modification time of the generated file is later,
  // the target file does not need to generate a new file.
  bool CheckModifyTime();

  void Generate();
  std::filesystem::path GetGeneratedFilePath();
};
} // namespace PReflTool
