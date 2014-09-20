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
    typedef std::vector<PCFGRule*> RulePointerVector;
    
private:
    typedef ProbabilisticContextFreeGrammar::LHSRange       PCFGRange;
    typedef ProbabilisticContextFreeGrammar::const_iterator PCFGCIt;
    typedef std::unordered_map<Symbol, RulePointerVector>   SymbolToRuleVectorMap;
    
  
public:    
    InsideOutsideCache(ProbabilisticContextFreeGrammar& pcfg) : grammar(pcfg) {
        build_rhs_index();
    }
    
private:
    
    void build_rhs_index() {
        // iterate over all non terminals...
        for (const Symbol& nt : grammar.get_nonterminals()) {
            // ... and their rules.
            PCFGRange rules_for_nt = grammar.rules_for(nt);
            for (PCFGCIt rule = rules_for_nt.first; rule != rules_for_nt.second; ++rule) {
                if (rule->arity() == 2) {
                    // map the first and the second symbol on the rhs to a vector containing a pointer to this rule
                    rhs_to_rule[(*rule)[0]].push_back(&(*rule));
                    rhs_to_rule[(*rule)[1]].push_back(&(*rule));
                }
            }
        }
    }
    
    
    
    
    
private:
    ProbabilisticContextFreeGrammar& grammar;
    SymbolToRuleVectorMap rhs_to_rule;
    
};

#endif	/* INSIDEOUTSIDECACHE_HPP */

