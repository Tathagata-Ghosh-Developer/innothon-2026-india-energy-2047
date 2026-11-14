#include "csv.hpp"

#include <cctype>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace e47 {

namespace {

// Splits one logical record. Handles quoted fields with embedded separators,
// doubled quotes and newlines (the caller supplies continuation lines).
bool parse_record(std::istream& in, char sep, std::vector<std::string>& out) {
  out.clear();
  std::string line;
  if (!std::getline(in, line)) return false;
  std::string field;
  bool quoted = false;
  for (;;) {
    for (size_t i = 0; i < line.size(); ++i) {
      const char c = line[i];
      if (quoted) {
        if (c == '"') {
          if (i + 1 < line.size() && line[i + 1] == '"') {
            field.push_back('"');
            ++i;
          } else {
            quoted = false;
          }
        } else {
          field.push_back(c);
        }
      } else if (c == '"') {
        quoted = true;
      } else if (c == sep) {
        out.push_back(std::move(field));
        field.clear();
      } else if (c != '\r') {
        field.push_back(c);
      }
    }
    if (!quoted) break;
    field.push_back('\n');
    if (!std::getline(in, line)) break;
  }
  out.push_back(std::move(field));
  return true;
}

}  // namespace

std::string trim(std::string_view s) {
  size_t b = 0, e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return std::string(s.substr(b, e - b));
}

std::string lower(std::string_view s) {
  std::string r(s);
  for (auto& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return r;
}

CsvTable CsvTable::read(const std::string& path, char sep, size_t skip_lines) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open " + path);
  // Skip a UTF-8 byte-order mark if present.
  char bom[3] = {0, 0, 0};
  in.read(bom, 3);
  if (!(in.gcount() == 3 && static_cast<unsigned char>(bom[0]) == 0xEF &&
        static_cast<unsigned char>(bom[1]) == 0xBB && static_cast<unsigned char>(bom[2]) == 0xBF)) {
    in.clear();
    in.seekg(0);
  }
  std::string skipped;
  for (size_t i = 0; i < skip_lines; ++i) std::getline(in, skipped);

  CsvTable t;
  std::vector<std::string> rec;
  if (!parse_record(in, sep, rec)) throw std::runtime_error("empty csv " + path);
  for (auto& h : rec) h = trim(h);
  // World Bank files end each row with a trailing separator; drop an empty last header.
  while (!rec.empty() && rec.back().empty()) rec.pop_back();
  t.header_ = rec;
  for (size_t i = 0; i < t.header_.size(); ++i) t.index_.emplace(t.header_[i], i);
  while (parse_record(in, sep, rec)) {
    if (rec.size() == 1 && rec[0].empty()) continue;
    rec.resize(t.header_.size());
    t.cells_.push_back(rec);
  }
  return t;
}

size_t CsvTable::col(std::string_view name) const {
  auto it = index_.find(std::string(name));
  if (it == index_.end()) throw std::runtime_error("missing column: " + std::string(name));
  return it->second;
}

double CsvTable::num(size_t row, size_t c) const {
  const std::string& s = cells_[row][c];
  if (s.empty()) return kNaN;
  const char* b = s.data();
  const char* e = b + s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(*b))) ++b;
  double v = 0;
  auto [p, ec] = std::from_chars(b, e, v);
  if (ec != std::errc() || p == b) return kNaN;
  return v;
}

struct CsvWriter::Impl {
  std::ofstream out;
};

CsvWriter::CsvWriter(const std::string& path) : impl_(new Impl) {
  impl_->out.open(path, std::ios::binary);
  if (!impl_->out) throw std::runtime_error("cannot write " + path);
}

CsvWriter::~CsvWriter() { delete impl_; }

void CsvWriter::sep() {
  if (!first_) impl_->out << ',';
  first_ = false;
}

CsvWriter& CsvWriter::field(std::string_view s) {
  sep();
  const bool needs_quote = s.find_first_of(",\"\n") != std::string_view::npos;
  if (!needs_quote) {
    impl_->out << s;
    return *this;
  }
  impl_->out << '"';
  for (char c : s) {
    if (c == '"') impl_->out << '"';
    impl_->out << c;
  }
  impl_->out << '"';
  return *this;
}

CsvWriter& CsvWriter::field(double v, int precision) {
  sep();
  if (std::isnan(v)) return *this;
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.*g", precision, v);
  impl_->out << buf;
  return *this;
}

CsvWriter& CsvWriter::field(long long v) {
  sep();
  impl_->out << v;
  return *this;
}

void CsvWriter::endrow() {
  impl_->out << '\n';
  first_ = true;
}

}  // namespace e47
