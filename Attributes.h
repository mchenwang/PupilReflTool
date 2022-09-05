#pragma once

#include <string>
#include <fstream>

namespace PReflTool {

struct Attr {
  // std::string name;
  Attr() = default;
  virtual ~Attr() = default;
  virtual std::string GetName() = 0;
  virtual void Write(std::ofstream&) = 0;
};

struct MetaAnnotate : public Attr {
  constexpr static char name[] = "meta";
  constexpr static const char *GetMarco() {
    return "META=clang::annotate(\"meta\")";
  }
  std::string GetName() override { return std::string{name}; }
  void Write(std::ofstream &out) override {
    out << "Attribute{ Name<\"" << name << "\">{} }";
  }
};

struct InfoAnnotate : public Attr {
  constexpr static char name[] = "info";
  constexpr static const char *GetMarco() {
    return "INFO(str)=clang::annotate(\"info\",str)";
  }
  std::string GetName() override { return std::string{name}; }

  std::string info;
  
  void Write(std::ofstream &out) override {
    out << "Attribute{ Name<\"" << name << "\">{}, \"" << info << "\"}";
  }
};

struct RangeAnnotate : public Attr {
  constexpr static char name[] = "range";
  constexpr static const char *GetMarco() {
    return "RANGE(a,b)=clang::annotate(\"range\",a,b)";
  }
  std::string GetName() override { return std::string{name}; }

  int cnt = 0;
  double vmin = 0.;
  double vmax = 0.;

  void Write(std::ofstream &out) override {
    out << "Attribute{ Name<\"" << name << "\">{}, std::make_pair(" << vmin
        << ", " << vmax << ") }";
  }

  void SetRange(double v) {
    if (cnt == 0) {
      vmin = v;
    } else if (cnt == 1) {
      vmax = v;

      if (vmin > vmax)
        std::swap(vmin, vmax);
    } else {
      if (v < vmin)
        vmin = v;
      if (v > vmax)
        vmax = v;
    }

    ++cnt;
  }
};

struct StepAnnotate : public Attr {
  constexpr static char name[] = "step";
  constexpr static const char *GetMarco() {
    return "STEP(x)=clang::annotate(\"step\",x)";
  }
  std::string GetName() override { return std::string{name}; }

  double step;

  void Write(std::ofstream &out) override {
    out << "Attribute{ Name<\"" << name << "\">{}, " << step << " }";
  }
};

} // namespace PReflTool