#ifndef __ENUM_H
#define __ENUM_H

/* Type-safe enumeration class with stringize functions */

#define ENUM_CLASS(EnumName, v0, ...) \
  struct EnumName {                   \
    enum Type { v0, __VA_ARGS__ , nType };    \
    Type t_;                          \
    EnumName(Type t=v0) : t_(t) {} \
    EnumName(const char* s) : t_() {  \
      for (int i=0;i<nType;++i) if (strcasecmp(names()[i],s)==0) { t_=Type(i); return; } \
      throw std::runtime_error("Unknown type string");                                   \
    }                                 \
    operator Type () const { return t_; }                    \
    operator char const* () const { return names()[t_]; }    \
  private:                                                   \
    template<typename T> operator T() const;                 \ 
    static char const* const* names() {                      \
      static char const* const str[] = { #v0 "," #__VA_ARGS "," }; \ 
      static char const* const str[] = {"Var","Factor","Edge","Tree",0}; \
      return str;                                            \
    }                                 \
  };                               







#endif
