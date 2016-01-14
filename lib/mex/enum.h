// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MEX_ENUM_H
#define __MEX_ENUM_H

#include <cstring>
#include <iostream>

/* Type-safe enumeration class with stringize functions */

#define MEX_ENUM(EnumName, v0, ...) \
  struct EnumName {                   \
    enum Type { v0, __VA_ARGS__ };    \
    Type t_;                          \
    EnumName(Type t=v0) : t_(t) {} \
    EnumName(const char* s) : t_() {  \
      /*for (int i=0;i<nType;++i) if (strcasecmp(names()[i],s)==0) { t_=Type(i); return; } */ \
      for (int i=0;names(i)[0]!=0;++i) if (strncmp(names(i),s,strlen(s))==0) { t_=Type(i); return; } \
      throw std::runtime_error("Unknown type string");                                   \
    }                                 \
    operator Type () const { return t_; }                    \
    operator char const* () const { return names(t_); }    \
    friend std::istream& operator >> (std::istream& is, EnumName& t) {    \
      std::string str; is >> str; t = EnumName(str.c_str()); return is; } \
    friend std::ostream& operator << (std::ostream& os, EnumName& t) {    \
      const char* s=(const char*)t; while (*s!=',') os<<*s++; return os; \
      /* os << (const char*)t; return os;                                */ \
    } \
  private:                                                   \
    /* template<typename T> operator T() const;                */ \
    static char const* names(unsigned int i) {        \
      static char const str[] = { #v0 "," #__VA_ARGS__ ",\0" }; \
      char const* s=str; while (*s!=0 && i!=0) if (*(s++)==',') --i; \
      return s;                                              \
    }                                 \
  };                               


#endif
