#pragma once
#include <chrono>
#include <regex>


namespace seeker {

using std::string;

class Time {
 public:
  static int64_t currentTime() {
    using namespace std::chrono;
    auto time_now = system_clock::now();
    auto durationIn = duration_cast<milliseconds>(time_now.time_since_epoch());
    return durationIn.count();
  };

  static int64_t microTime() {
    using namespace std::chrono;
    auto time_now = system_clock::now();
    auto durationIn = duration_cast<microseconds>(time_now.time_since_epoch());
    return durationIn.count();
  };
};

class String {
 public:
  static string toLower(const string& target) {
    string out{};
    for (auto c : target) {
      out += ::tolower(c);
    }
    return out;
  }

  static string toUpper(const string& target) {
    string out{};
    for (auto c : target) {
      out += ::toupper(c);
    }
    return out;
  }

  static string trim(string& s) {
    string rst{s};
    if (rst.empty()) {
      return rst;
    }
    rst.erase(0, rst.find_first_not_of(" "));
    rst.erase(s.find_last_not_of(" ") + 1);
    return rst;
  }

  static std::vector<string> split(const string& target, const string& sp) {
    std::vector<string> rst{};
    if (target.size() == 0) {
      return rst;
    }

    const auto spLen = sp.length();
    string::size_type pos = 0;
    auto f = target.find(sp, pos);

    while (f != string::npos) {
      auto r = target.substr(pos, f - pos);
      rst.emplace_back(r);
      pos = f + spLen;
      f = target.find(sp, pos);
    }
    rst.emplace_back(target.substr(pos, target.length()));
    return rst;
  }

  static string removeBlanks(const string& target) {
    static std::regex blankRe{R"(\s+)"};
    return std::regex_replace(target, blankRe, "");
  }

  static string removeLastEmptyLines(const string& target) {
    size_t len = target.length();
    size_t i = len - 1;
    for (; i > 0; i--) {
      if (target[i] == '\n') {
        continue;
      } else if (target[i] == '\r') {
        continue;
      } else {
        break;
      }
    }
    return target.substr(0, i + 1);
  }
};


class ByteArray {
 public:
  template <typename T>
  static void writeData(uint8_t* buf, T num) {
    int len(sizeof(T));
    for (size_t i = 0; i < len; ++i) {
      buf[i] = (uint8_t)(num >> (i * 8));
    }
  }

  template <typename T>
  static void readData(uint8_t* buf, T& num) {
    uint8_t len(sizeof(T));
    static uint8_t byMask(0xFF);
    num = 0;

    for (size_t i = 0; i < len; ++i) {
      num <<= 8;
      num |= (T)(buf[len - 1 - i] & byMask);
    }
  }
};


}  // namespace seeker