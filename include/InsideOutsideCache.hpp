/* 
 * File:   InsideOutsideCache.hpp
 * Author: johannes
 *
 * Created on September 20, 2014, 2:28 AM
 */

#ifndef INSIDEOUTSIDECACHE_HPP
#define	INSIDEOUTSIDECACHE_HPP

#include "ProbabilisticContextFreeGrammar.hpp"
#include "PCFGRule.hpp"

#include <vector>
#include <unordered_map>

class InsideOutsideCache {
public:
    typedef ProbabilisticContextFreeGrammar::Symbol     Symbol;
    
private:
    typedef ProbabilisticContextFreeGrammar::LHSRange       PCFGRange;
    typedef ProbabilisticContextFreeGrammar::const_iterator PCFGCIt;
    
    
public:    
    InsideOutsideCache(ProbabilisticContextFreeGrammar& pcfg) : grammar(pcfg) {}
    
    
    
private:
    ProbabilisticContextFreeGrammar& grammar;
};

#endif	/* INSIDEOUTSIDECACHE_HPP */

