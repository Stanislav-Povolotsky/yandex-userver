#pragma once

#include <chrono>
#include <exception>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>
#include <type_traits>

#include <formats/common/meta.hpp>
#include <logging/level.hpp>
#include <logging/log_extra.hpp>
#include <logging/logger.hpp>
#include <utils/encoding/hex.hpp>
#include <utils/encoding/tskv.hpp>
#include <utils/meta.hpp>

/// @file logging/log_helper.hpp
/// @brief @copybrief logging::LogHelper

namespace logging {

namespace impl {

struct Noop {};

struct HexBase {
  uint64_t value;

  template <typename Unsigned,
            typename = std::enable_if_t<std::is_unsigned_v<Unsigned>>>
  explicit HexBase(Unsigned value) : value(value) {
    static_assert(sizeof(Unsigned) <= sizeof(value));
  }

  template <typename T>
  explicit HexBase(T* pointer) : HexBase(reinterpret_cast<uintptr_t>(pointer)) {
    static_assert(sizeof(value) == sizeof(uintptr_t));
  }
};

}  // namespace impl

/// Formats value in a hex mode with the fixed length representation.
struct Hex final : impl::HexBase {
  using impl::HexBase::HexBase;
};

/// Formats value in a hex mode with the shortest representation, just like
/// std::to_chars does.
struct HexShort final : impl::HexBase {
  using impl::HexBase::HexBase;
};

/// Stream-like tskv-formatted log message builder.
///
/// Users can add LogHelper& operator<<(LogHelper&, ) overloads to use a faster
/// localeless logging, rather than outputting data through the ostream
/// operator.
class LogHelper final {
 public:
  enum class Mode { kDefault, kNoSpan };

  /// @brief Constructs LogHelper with span logging
  /// @param level message log level
  /// @param path path of the source file that generated the message
  /// @param line line of the source file that generated the message
  /// @param func name of the function that generated the message
  /// @param mode logging mode - with or without span
  LogHelper(LoggerPtr logger, Level level, std::string_view path, int line,
            std::string_view func, Mode mode = Mode::kDefault) noexcept;

  ~LogHelper();

  LogHelper(LogHelper&&) = delete;
  LogHelper(const LogHelper&) = delete;
  LogHelper& operator=(LogHelper&&) = delete;
  LogHelper& operator=(const LogHelper&) = delete;

  // Helper function that could be called on LogHelper&& to get LogHelper&.
  LogHelper& AsLvalue() noexcept { return *this; }

  bool IsLimitReached() const;

  template <typename T>
  LogHelper& operator<<(const T& value) {
    constexpr bool encode = utils::encoding::TypeNeedsEncodeTskv<T>::value;
    EncodingGuard guard{*this, encode ? Encode::kValue : Encode::kNone};

    // if constexpr FP, maybe fixed in llvm 10
    if constexpr (std::is_same_v<T, char> ||
                  // NOLINTNEXTLINE(bugprone-branch-clone)
                  std::is_constructible_v<std::string_view, T>) {
      Put(value);
    } else if constexpr (std::is_floating_point_v<T>) {
      PutFloatingPoint(value);
    } else if constexpr (std::is_signed_v<T>) {
      PutSigned(value);
    } else if constexpr (std::is_same_v<T, bool>) {
      PutBoolean(value);
    } else if constexpr (std::is_unsigned_v<T>) {
      PutUnsigned(value);
    } else if constexpr (std::is_base_of_v<std::exception, T>) {
      PutException(value);
    } else if constexpr (meta::kIsOstreamWritable<T>) {
      Stream() << value;
    } else if constexpr (meta::kIsRange<T> &&
                         !formats::common::kIsFormatValue<T>) {
      PutRange(value);
    } else {
      static_assert(!sizeof(T),
                    "Please implement logging for your type: "
                    "logging::LogHelper& operator<<(logging::LogHelper& lh, "
                    "const T& value)");
    }

    return *this;
  }

  /// Extends internal LogExtra
  LogHelper& operator<<(const LogExtra& extra);

  /// Extends internal LogExtra
  LogHelper& operator<<(LogExtra&& extra);

  LogHelper& operator<<(Hex hex);

  LogHelper& operator<<(HexShort hex);

  /// @cond
  // For internal use only!
  operator impl::Noop() const noexcept { return {}; }
  /// @endcond

 private:
  enum class Encode { kNone, kValue, kKeyReplacePeriod };

  struct EncodingGuard {
    LogHelper& lh;

    EncodingGuard(LogHelper& lh, Encode mode) noexcept;
    ~EncodingGuard();
  };

  void DoLog() noexcept;

  void AppendLogExtra();
  void LogTextKey();
  void LogModule(std::string_view path, int line, std::string_view func);
  void LogIds();
  void LogSpan();

  void PutFloatingPoint(float value);
  void PutFloatingPoint(double value);
  void PutFloatingPoint(long double value);
  void PutUnsigned(unsigned long long value);
  void PutSigned(long long value);
  void PutBoolean(bool value);
  void Put(std::string_view value);
  void Put(char value);
  void PutException(const std::exception& ex);
  void PutQuoted(std::string_view value);

  template <typename T>
  void PutRangeElement(const T& value);

  template <typename T, typename U>
  void PutMapElement(const std::pair<const T, U>& value);

  template <typename T>
  void PutRange(const T& range);

  std::ostream& Stream();

  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

inline LogHelper& operator<<(LogHelper& lh, std::error_code ec) {
  lh << ec.category().name() << ':' << ec.value() << " (" << ec.message()
     << ')';
  return lh;
}

template <typename T>
LogHelper& operator<<(LogHelper& lh, const std::atomic<T>& value) {
  return lh << value.load();
}

template <typename T>
LogHelper& operator<<(LogHelper& lh, const T* value) {
  if (value == nullptr) {
    lh << "(null)";
  } else if constexpr (std::is_same_v<T, char>) {
    lh << std::string_view(value);
  } else {
    lh << Hex{value};
  }
  return lh;
}

template <typename T>
LogHelper& operator<<(LogHelper& lh, T* value) {
  return lh << static_cast<const T*>(value);
}

template <typename T>
LogHelper& operator<<(LogHelper& lh, const std::optional<T>& value) {
  if (value)
    lh << *value;
  else
    lh << "(none)";
  return lh;
}

template <class Result, class... Args>
LogHelper& operator<<(LogHelper& lh, Result (*)(Args...)) {
  static_assert(!sizeof(Result),
                "Outputting functions or std::ostream formatters is forbidden");
  return lh;
}

LogHelper& operator<<(LogHelper& lh, std::chrono::system_clock::time_point tp);
LogHelper& operator<<(LogHelper& lh, std::chrono::seconds value);
LogHelper& operator<<(LogHelper& lh, std::chrono::milliseconds value);
LogHelper& operator<<(LogHelper& lh, std::chrono::microseconds value);
LogHelper& operator<<(LogHelper& lh, std::chrono::nanoseconds value);
LogHelper& operator<<(LogHelper& lh, std::chrono::minutes value);
LogHelper& operator<<(LogHelper& lh, std::chrono::nanoseconds value);
LogHelper& operator<<(LogHelper& lh, std::chrono::hours value);

template <typename T>
void LogHelper::PutRangeElement(const T& value) {
  if constexpr (std::is_constructible_v<std::string_view, T>) {
    PutQuoted(value);
  } else {
    *this << value;
  }
}

template <typename T, typename U>
void LogHelper::PutMapElement(const std::pair<const T, U>& value) {
  PutRangeElement(value.first);
  Put(": ");
  PutRangeElement(value.second);
}

template <typename T>
void LogHelper::PutRange(const T& range) {
  static_assert(meta::kIsRange<T>);
  using std::begin;
  using std::end;

  constexpr std::string_view kSeparator = ", ";
  Put('[');

  bool is_first = true;
  auto curr = begin(range);
  const auto end_iter = end(range);

  while (curr != end_iter) {
    if (IsLimitReached()) {
      break;
    }
    if (is_first) {
      is_first = false;
    } else {
      Put(kSeparator);
    }

    if constexpr (meta::kIsMap<T>) {
      PutMapElement(*curr);
    } else {
      PutRangeElement(*curr);
    }
    ++curr;
  }

  const auto extra_elements = std::distance(curr, end_iter);

  if (extra_elements != 0) {
    if (!is_first) {
      *this << kSeparator;
    }
    *this << "..." << extra_elements << " more";
  }

  Put(']');
}

}  // namespace logging