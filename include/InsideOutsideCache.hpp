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
#include <bitset>

#include "easylogging++.h"


class InsideOutsideCache {
public:
    typedef ProbabilisticContextFreeGrammar::Symbol     Symbol;
    typedef std::vector<const PCFGRule*>                RulePointerVector;
    typedef uint8_t                                     LengthType;
    typedef double                                      InsideOutsideProbability;
        
        
private:
    typedef ProbabilisticContextFreeGrammar::LHSRange                 PCFGRange;
    typedef ProbabilisticContextFreeGrammar::const_iterator           PCFGCIt;
    typedef std::unordered_map<Symbol, RulePointerVector>             SymbolToRuleVectorMap;
    typedef uint64_t                                                  CachedItem;
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
    
    inline const RulePointerVector* const get_rules_for_first_symbol(const Symbol& first_symbol) const {
        SymbolToRuleVectorMap::const_iterator cit = first_symbol_rules.find(first_symbol);
        return cit != first_symbol_rules.end()? &(cit->second) : nullptr;
    }

    inline const RulePointerVector* const get_rules_for_second_symbol(const Symbol& second_symbol) const {
        SymbolToRuleVectorMap::const_iterator cit = second_symbol_rules.find(second_symbol);
        return cit != second_symbol_rules.end()? &(cit->second) : nullptr;    
    }
    
    inline const InsideOutsideProbability* const get_inside_cache(const Symbol& symbol, const LengthType& begin, const LengthType& end) const {
        CachedItem key = create_inside_key(symbol, begin, end);
        CacheMap::const_iterator cit = inside_cache.find(key);
        return cit != inside_cache.end() ? &(cit->second) : nullptr;
    }

    inline const InsideOutsideProbability* const get_outside_cache(const Symbol& symbol, const LengthType& begin, const LengthType& end) const {
        CachedItem key = create_inside_key(symbol, begin, end);
        CacheMap::const_iterator cit = outside_cache.find(key);
        return cit != outside_cache.end() ? &(cit->second) : nullptr;
    }
    
    inline void store_inside_cache(const Symbol& symbol, const LengthType& begin, const LengthType& end, const InsideOutsideProbability& value) {
        inside_cache[create_inside_key(symbol, begin, end)] = value;
    }

    inline void store_outside_cache(const Symbol& symbol, const LengthType& begin, const LengthType& end, const InsideOutsideProbability& value) {
        outside_cache[create_outside_key(symbol, begin, end)] = value;
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

    CachedItem create_inside_key(const Symbol& symbol, const LengthType& begin, const LengthType& end) const {
        CachedItem buffer = symbol;     // assign the first 32bit in the buffer for the symbol
        buffer = (buffer << 8) | begin; // Now push the next 8bits in
        buffer = (buffer << 8) | end;   // and another 8bits.
        LOG(INFO) << "InsideOutsideCache: Converting Inside<" << symbol << "," << (int)begin << "," << (int)end << "> to 64bit key: " << (std::bitset<64>) buffer;
        return buffer;
        
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

    CachedItem create_outside_key(const Symbol& symbol, const LengthType& begin, const LengthType& end) const {
        CachedItem buffer = symbol;     // assign the first 32bit in the buffer for the symbol
        buffer = (buffer << 8) | begin; // Now push the next 8bits in
        buffer = (buffer << 8) | end;   // and another 8bits.
        LOG(INFO) << "InsideOutsideCache: Converting Outside <" << symbol << "," << (int)begin << "," << (int)end << "> to 64bit key: " << (std::bitset<64>) buffer;
        return buffer;
    }
    
    
    
    
    
    
private:
    ProbabilisticContextFreeGrammar& grammar;
    SymbolToRuleVectorMap first_symbol_rules;
    SymbolToRuleVectorMap second_symbol_rules;

    CacheMap inside_cache;
    CacheMap outside_cache;
    
};

#endif	/* INSIDEOUTSIDECACHE_HPP */

