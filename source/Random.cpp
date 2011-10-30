/*
 * random.cpp
 *
 *  Created on: Oct 26, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "_base.h"

/*
 * Random number generator is static, implemented in _base.h.
 * Here only the static member variables are initialized.
 */
int rand::state = 1;
boost::minstd_rand rand::_r;


