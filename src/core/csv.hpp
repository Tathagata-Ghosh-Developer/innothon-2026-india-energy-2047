#pragma once
// Minimal RFC 4180 CSV reader and writer.
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace e47 {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

class CsvTable {
 public:
  static CsvTable read(const std::string& path, char sep = ',', size_t skip_lines = 0);

  size_t rows() const { return cells_.size(); }
  size_t cols() const { return header_.size(); }
  const std::vector<std::string>& header() const { return header_; }

  bool has(std::string_view col) const { return index_.count(std::string(col)) > 0; }
  size_t col(std::string_view name) const;  // throws if missing

  const std::string& str(size_t row, size_t col) const { return cells_[row][col]; }
  double num(size_t row, size_t col) const;  // NaN for empty or non-numeric

 private:
  std::vector<std::string> header_;
  std::unordered_map<std::string, size_t> index_;
  std::vector<std::vector<std::string>> cells_;
};

class CsvWriter {
 public:
  explicit CsvWriter(const std::string& path);
  ~CsvWriter();
  CsvWriter(const CsvWriter&) = delete;
  CsvWriter& operator=(const CsvWriter&) = delete;

  CsvWriter& field(std::string_view s);
  CsvWriter& field(double v, int precision = 6);
  CsvWriter& field(long long v);
  CsvWriter& field(int v) { return field(static_cast<long long>(v)); }
  void endrow();

 private:
  void sep();
  struct Impl;
  Impl* impl_;
  bool first_ = true;
};

std::string trim(std::string_view s);
std::string lower(std::string_view s);

}  // namespace e47
