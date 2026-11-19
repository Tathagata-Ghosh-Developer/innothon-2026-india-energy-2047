#pragma once
// A costly computation run on a worker thread only when it is asked for, and run again only
// when its inputs (the key) change. The dashboard uses it for the mix sensitivity (12 LP
// solves), which only its mix evidence view shows.
#include <chrono>
#include <exception>
#include <future>
#include <optional>
#include <string>
#include <utility>

namespace e47 {

template <class Key, class Value>
class CachedTask {
 public:
  enum class State { kRunning, kReady, kFailed };

  CachedTask() = default;
  CachedTask(const CachedTask&) = delete;
  CachedTask& operator=(const CachedTask&) = delete;
  ~CachedTask() { wait(); }

  // State of the value for `key`. kReady: value() holds it. kFailed: fn threw for this key and
  // error() says why (the failure is cached like a value). kRunning: it is being computed. When
  // neither the value nor a failure for `key` is held and nothing is running, fn starts on a
  // worker thread. A run for an older key cannot be stopped: it is let finish, and the first
  // request after that starts the run for the new key. Never blocks.
  template <class Fn>
  State request(const Key& key, Fn&& fn) {
    collect(false);
    if (key_ && *key_ == key) return error_.empty() ? State::kReady : State::kFailed;
    if (!job_.valid()) {
      job_key_ = key;
      job_ = std::async(std::launch::async, std::forward<Fn>(fn));
      ++runs_;
    }
    return State::kRunning;
  }

  const Value& value() const { return value_; }
  const std::string& error() const { return error_; }
  // True while a computation runs (finished but not yet collected counts as not running).
  bool busy() const { return job_.valid() && job_.wait_for(std::chrono::seconds(0)) != std::future_status::ready; }
  int runs() const { return runs_; }  // computations started so far
  // Blocks until a running computation has finished, then collects it.
  void wait() { collect(true); }

 private:
  void collect(bool block) {
    if (!job_.valid()) return;
    if (!block && job_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    key_ = job_key_;
    try {
      value_ = job_.get();
      error_.clear();
    } catch (const std::exception& e) {
      value_ = Value{};
      error_ = *e.what() ? e.what() : "unknown error";
    } catch (...) {
      value_ = Value{};
      error_ = "unknown error";
    }
  }

  std::future<Value> job_;
  Key job_key_{};
  std::optional<Key> key_;  // key of value_ and error_
  Value value_{};
  std::string error_;
  int runs_ = 0;
};

}  // namespace e47
