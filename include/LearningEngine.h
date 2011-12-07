/*
 * LearningEngine.h
 *
 *  Copyright (C) 2011 Lars Otten
 *  Licensed under the MIT License, see LICENSE.TXT
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

#endif /* PARALLEL_STATIC */
#endif /* LEARNINGENGINE_H_ */
