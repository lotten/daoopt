/*
 * debug.h
 *
 *  Created on: Nov 13, 2008
 *      Author: lars
 */

#include <iostream>
#include "_base.h"

#ifndef DEBUG_H_
#define DEBUG_H_

inline void myprint(std::string s) {
  {
    GETLOCK(mtx_io, lk);
    std::cout << s << std::flush;
  }
}

/*
inline void myprint(char* c) {
  {
    GETLOCK(mtx_io, lk);
    std::string s(c);
    std::cout << s << std::flush;
  }
}
*/

inline void myerror(std::string s) {
  {
    GETLOCK(mtx_io, lk);
    std::cerr << s << std::flush;
  }
}

/*
inline void myerror(char* c) {
  {
    GETLOCK(mtx_io, lk);
    std::string s(c);
    std::cerr << s << std::flush;
  }
}
*/

#endif /* DEBUG_H_ */
