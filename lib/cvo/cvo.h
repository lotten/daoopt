/*
 * cvo.h
 *
 *  Created on: Dec 14, 2011
 *      Author: kalev
 */

#ifndef CVO_H_
#define CVO_H_

#include <stdlib.h>

#include "cvo/ARP/ARPall.hxx"

/*
 * Interface for CVO ordering code.
 */
namespace CVO {

// e.g. CVO::Sample_SingleThreaded_MinFill("E:\\UCI\\linkage\\pedigree9.uai", 1000) ;
int Sample_SingleThreaded_MinFill(const std::string & uaifile, int nIterations) ;

} ;


#endif /* CVO_H_ */
