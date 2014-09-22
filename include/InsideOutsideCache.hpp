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
#include <limits>       // std::numeric_limits
#include <cstdint>

class InsideOutsideCache {
public:
    typedef ProbabilisticContextFreeGrammar::Symbol     Symbol;
    typedef std::vector<const PCFGRule*>                RulePointerVector;
    typedef uint8_t                                     LengthType;
    typedef uint16_t                                    CachedItem;
    typedef double                                      InsideOutsideProbability;
        
        
private:
    typedef ProbabilisticContextFreeGrammar::LHSRange                 PCFGRange;
    typedef ProbabilisticContextFreeGrammar::const_iterator           PCFGCIt;
    typedef std::unordered_map<Symbol, RulePointerVector>             SymbolToRuleVectorMap;
    typedef std::unordered_map<CachedItem, InsideOutsideProbability>  CacheMap;
    
    
public:    
    InsideOutsideCache(ProbabilisticContextFreeGrammar& pcfg) : grammar(pcfg) {
        assert(std::numeric_limits<Symbol>::max() <= UINT32_MAX); 
        // check, that the datatype that is used for symbols is not bigger than 32bit (eventhough it could be a 48bit type). 
        // This is needed for fast caching of <Symbol, Begin, End> and <Symbol, Length> Typles
        build_rhs_index();
    }

    const ProbabilisticContextFreeGrammar& get_grammar() {
        return grammar;
    }
    
    inline const RulePointerVector& get_rules_for_first_symbol(const Symbol& first_symbol) const {
        return first_symbol_rules.find(first_symbol)->second;
    }

    inline const RulePointerVector& get_rules_for_second_symbol(const Symbol& second_symbol) const {
        return second_symbol_rules.find(second_symbol)->second;
    }
    
    

    
    inline CachedItem create_inside_key(const Symbol& symbol, const LengthType& begin, const LengthType& end) const {
        CachedItem buffer = symbol;     // assign the first 32bit in the buffer for the symbol
        buffer = (buffer << 8) | begin; // Now push the next 8bits in
        return (buffer << 8) | end;     // and another 8bits.
        
        /* Example:
         * Assuming that Symbol is a 32bit type and LengthType is 8bit. 
         * For a better visualisation let's also assume, that the value of 'symbol'
         * is '4294967295' - the maximum number that a 32bit type can store. 
         * Let's assign the value '0' to 'begin' and '255' to end;
         * In binary, the variables would look like this:
         * symbol = 11111111111111111111111111111111
         * begin  = 00000000
         * end    = 11111111
         * 
         * In the first step, all 32 bits of 'symbol' are copied to the buffer. 
         * The buffer now has the value: 
         * 0000000000000000000000000000000011111111111111111111111111111111
         * After this, 'begin' is 'attached' to the buffer:
         * 0000000000000000000000001111111111111111111111111111111100000000
         * And now, 'end':
         * 0000000000000000111111111111111111111111111111110000000011111111
         * 
         * In this way, it is possible to create a unique, hash-like value that can
         * be used as a key in a map to find the already calculated inside probability
         * for the symbol, begin and end. 
         */
    }
    
    inline CachedItem create_outside_key(const Symbol& symbol, const LengthType& length) const {
        return (symbol << 8) | length;   // Concatenate the bits of 
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
                    first_symbol_rules[(*rule)[0]].push_back(&(*rule));
                    second_symbol_rules[(*rule)[1]].push_back(&(*rule));
                }
            }
        }
    }
    
    
    
    
    
    
    
private:
    ProbabilisticContextFreeGrammar& grammar;
    SymbolToRuleVectorMap first_symbol_rules;
    SymbolToRuleVectorMap second_symbol_rules;


    
};

#endif	/* INSIDEOUTSIDECACHE_HPP */

