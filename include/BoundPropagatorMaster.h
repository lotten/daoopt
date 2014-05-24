/*
 * BoundPropagatorMaster.h
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
 *  Created on: Feb 14, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef BOUNDPROPAGATORMASTER_H_
#define BOUNDPROPAGATORMASTER_H_

#include "BoundPropagator.h"

#ifdef PARALLEL_DYNAMIC

namespace daoopt {

class BoundPropagatorMaster : public BoundPropagator {

protected:
  /* same as m_space, but different type */
  SearchSpaceMaster* m_spaceMaster;

public:
  /* threading operator for master process */
  void operator () ();

protected:
  bool isMaster() const { return true; }

public:
  /* simple constructor */
  BoundPropagatorMaster(Problem* p, SearchSpaceMaster* s) : BoundPropagator(p,s), m_spaceMaster(s) {}

};

}  // namespace daoopt

#endif /* PARALLEL_DYNAMIC */

#endif /* BOUNDPROPAGATORMASTER_H_ */
