/*
 * random.cpp
 *
 *  Copyright (C) 2008-2012 Lars Otten
 *  This file is part of DAOOPT.
 *
 *  DAOOPT is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  DAOOPT is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DAOOPT.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *  Created on: Oct 26, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "_base.h"

namespace daoopt {
/*
 * Random number generator is static, implemented in _base.h.
 * Here only the static member variables are initialized.
 */

int rand::state = 1;
boost::minstd_rand rand::_r;

}  // namespace daoopt
