/*
* \brief  libeho library 
* \author brfc
* \date   2024-05-08
*
* Copyright (C) 2024 R Carvalho
*
* This file is an libecho implementation,
* which is distributed under the terms of the MIT License.
*
*/

#pragma once

#include <iostream>
#include <chrono>
#include <type_traits>
#include <sys/resource.h>
#include <libunwind.h>
#include <cxxabi.h>
#include <list>
#include <utility>


const std::string ANSI_META = "\x1b[37m";
const std::string ANSI_EXPR = "\x1b[38;5;3m";
const std::string ANSI_DEFAULT = "\033[0m";
const std::string ANSI_VALUE = "\x1b[01m";
const std::string ANSI_TYPE = "\x1b[32m";
const std::string ANSI_PURPLE = "\e[0;35m";
const std::string ANSI_RED = "\e[1;31m";
const std::string ANSI_GREEN_ARROW = "\033[1;33m->\033[0m";



class Echo
{
private:
  using str = std::string;
  str _file;
  str _fun;
  int _line;
  std::ostream &_out;

  str _info() const
  {
    return "[" + _file + ":" + std::to_string(_line) + "(" + _fun + ")]";
  }

  void _write_meta() const 
  {
    _out << ANSI_META << _info() << ANSI_DEFAULT;
  }

  void _write_expression(const std::string& expr) const
  {
    _out << ANSI_EXPR << " " << expr << ANSI_DEFAULT;
  }

  bool _is_str_of_str(const str &expr) const noexcept
  {
    return expr.front() == '"' && expr.back() == '"';
  }

  void _write_bad_expr(const str &expr) const noexcept
  {
    _write_meta();

    _out << ANSI_RED << " " << \
      expr << ANSI_META     << \
      " = False"            << \
      ANSI_DEFAULT << std::endl;
  }

  void _write(const str &expr) const noexcept
  {
    _write_meta();
    _write_expression(expr);
    _out << std::endl;
  }

  template <typename T>
  void _write(const str &expr, const str &name, const T &value) const noexcept
  {
    _write_meta();
    _write_expression(expr);
    
    _out << ANSI_META     <<  \
      " = " << ANSI_VALUE <<  \
      value << ANSI_TYPE  <<  \
      "(" << name << ")"  <<  \
      ANSI_DEFAULT << std::endl;
  }

  template <typename T>
  auto _type_name_map()
  {
    return typeid(T).name();
  }

  template <>
  auto _type_name_map<bool>()
  {
    return "bool";
  }

  template <>
  auto _type_name_map<int>()
  {
    return "int";
  }

  template <>
  auto _type_name_map<const char *>()
  {
    return "const char*";
  }

  template <typename T>
  auto _stringify_value(T v)
  {
    return v;
  }

  template <>
  auto _stringify_value<bool>(bool v)
  {
    return v ? "True" : "False";
  }

public:
  Echo(str file, str fun, int line) : 
    _file(std::move(file)),
    _fun(std::move(fun)),
    _line(line),
    _out(std::cout) 
  {}

  template <typename T>
  auto make_printable(T value)
  {
    return std::make_pair(_type_name_map<T>(), _stringify_value<T>(value));
  };

  template <typename T>
  auto pprint(str expr, T value)
  {
    auto type_value = make_printable<T>(value);
    if (_is_str_of_str(expr))
      _write(expr);
    else
      _write(expr, type_value.first, type_value.second);
  }

  void pprint_stack(const std::list<std::string> &l)
  {
    for (const auto &el : l)
      _out << ANSI_GREEN_ARROW << ANSI_META << "\t" << el << ANSI_DEFAULT << std::endl;
  };

  void pprint_expect(const std::string &expr, bool condition) 
  {
    if (condition) {
      pprint(expr, condition);
      return;
    }
    _write_bad_expr(expr);

    unw_cursor_t cursor;
    unw_context_t context;

    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    std::list<std::string> syms;
    while (unw_step(&cursor) > 0)
    {
      unw_word_t offset, pc;
      unw_get_reg(&cursor, UNW_REG_IP, &pc);
      if (pc == 0)
        break;
      
      char sym[256];
      if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
        int status;
        char *demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
        if (status == 0) {
          std::string copy = demangled;
          syms.push_back(copy);
          free(demangled);
        }
        else 
          syms.push_back(sym);
      }
      else
        _out << "    [unknown]" << std::endl;
    }

    pprint_stack(syms);
  };
};

#define log(x) Echo(__FILE__, __func__, __LINE__).pprint(#x, x);
#define expect(condition) Echo(__FILE__, __func__, __LINE__).pprint_expect(#condition, condition);

size_t getCurrentMemoryUsage()
{
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  return usage.ru_maxrss * 1024;
}

void perf_header(std::ostream &os, std::string file, int line, std::string fun) noexcept
{
  os << ANSI_META << "[" << file << ":" << std::to_string(line) << \
    "(" << fun << ")" << "]" << ANSI_PURPLE << " PERF START"    << \
    ANSI_DEFAULT << std::endl;
}

template <typename T, typename U>
void perf_output(std::ostream &os, T time, U mem) noexcept
{
  os << ANSI_META << " ------------------------ " << std::endl;
  os << ANSI_META << "|" << ANSI_PURPLE << " PERF " << ANSI_META << "Report:" << std::endl;
  os << ANSI_META << " ------------------------ " << std::endl;
  os << ANSI_META << "|\tRTime: " << time << " ms" << std::endl;
  os << ANSI_META << "|\tRAM inc: " << mem << " bytes" << std::endl;
  os << ANSI_META << " ------------------------ " << ANSI_DEFAULT << std::endl;
}

#define perf(code)                                                                                        \
  do                                                                                                      \
  {                                                                                                       \
    perf_header(std::cout, __FILE__, __LINE__, __func__);                                                 \
    auto start_time = std::chrono::high_resolution_clock::now();                                          \
    auto start_memory = getCurrentMemoryUsage();                                                          \
    code auto end_time = std::chrono::high_resolution_clock::now();                                       \
    auto end_memory = getCurrentMemoryUsage();                                                            \
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count(); \
    auto memory = end_memory - start_memory;                                                              \
    perf_output(std::cout, duration, memory);                                                             \
  } while (0);
