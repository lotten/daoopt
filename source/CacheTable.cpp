/*
 * CacheTable.cpp
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
 *  Created on: Feb 15, 2012
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "CacheTable.h"

namespace daoopt {

#ifndef NO_ASSIGNMENT
  const pair<const double, const vector<val_t> > CacheTable::NOT_FOUND = make_pair(ELEM_NAN, vector<val_t>());
#else
  const double CacheTable::NOT_FOUND = ELEM_NAN;
#endif

}  // namespace daoopt
