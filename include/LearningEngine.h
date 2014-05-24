/*
 * LearningEngine.h
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
 *  Created on: Nov 3, 2011
 *      Author: lars
 */

#ifndef LEARNINGENGINE_H_
#define LEARNINGENGINE_H_

#include "_base.h"

#ifdef PARALLEL_STATIC
#include "ProgramOptions.h"
#include "Statistics.h"

namespace daoopt {

class LearningEngine {
protected:
  ProgramOptions* m_options;
  vector<SubproblemStats> m_samples;

public:
  /* Adds a subproblem (i.e. its stats) to the learning set */
  void addSample(const SubproblemStats& sample) { m_samples.push_back(sample); }

  /* Trains the model */
  virtual bool trainModel() = 0;

  /* Uses the trained model to predict the complexity of the subproblem
   * with the given stats */
  virtual double predict(const SubproblemStats&) const = 0;

  /* Prints subproblem stats to file (with column titles) */
  void statsToFile(const string& fn) const;

  LearningEngine(ProgramOptions* opt) : m_options(opt) {}
  virtual ~LearningEngine() {};

};

class LinearRegressionLearner : public LearningEngine {
public:
  bool trainModel() { return false; }  // TODO
  double predict(const SubproblemStats&) const { return 0.0; }  // TODO
  LinearRegressionLearner(ProgramOptions* opt) : LearningEngine(opt) {}
};

}  // namespace daoopt

#endif /* PARALLEL_STATIC */

#endif /* LEARNINGENGINE_H_ */
