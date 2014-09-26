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
    typedef uint8_t                                     LengthType;
    typedef double                                      InsideOutsideProbability;
        
        
private:
    typedef ProbabilisticContextFreeGrammar::LHSRange                 PCFGRange;
    typedef ProbabilisticContextFreeGrammar::const_iterator           PCFGCIt;
    typedef uint64_t                                                  CachedItem;
    typedef std::unordered_map<CachedItem, InsideOutsideProbability>  CacheMap;
    
    
public:    
    InsideOutsideCache(ProbabilisticContextFreeGrammar& pcfg) : grammar(pcfg) {
        assert(std::numeric_limits<Symbol>::max() <= UINT32_MAX); 
        // check, that the datatype that is used for symbols is not bigger than 32bit (eventhough it could be a 48bit type). 
        // This is needed for fast caching of <Symbol, Begin, End> and <Symbol, Length> Typles
    }

    const ProbabilisticContextFreeGrammar& get_grammar() {
        return grammar;
    }
   
    inline const InsideOutsideProbability* const get_inside_cache(const Symbol& symbol, const LengthType& begin, const LengthType& end) const {
        CachedItem key = create_key(symbol, begin, end);
        CacheMap::const_iterator cit = inside_cache.find(key);
        return cit != inside_cache.end() ? &(cit->second) : nullptr;
    }

    inline const InsideOutsideProbability* const get_outside_cache(const Symbol& symbol, const LengthType& begin, const LengthType& end) const {
        CachedItem key = create_key(symbol, begin, end);
        CacheMap::const_iterator cit = outside_cache.find(key);
        return cit != outside_cache.end() ? &(cit->second) : nullptr;
    }
    
    inline void store_inside_cache(const Symbol& symbol, const LengthType& begin, const LengthType& end, const InsideOutsideProbability& value) {
        inside_cache[create_key(symbol, begin, end)] = value;
    }

    inline void store_outside_cache(const Symbol& symbol, const LengthType& begin, const LengthType& end, const InsideOutsideProbability& value) {
        outside_cache[create_key(symbol, begin, end)] = value;
    }
    
private:
    
    CachedItem create_key(const Symbol& symbol, const LengthType& begin, const LengthType& end) const {
        CachedItem buffer = symbol;     // assign the first 32bit in the buffer for the symbol
        buffer = (buffer << 8) | begin; // Now push the next 8bits in
        buffer = (buffer << 8) | end;   // and another 8bits.
        VLOG(8) << "InsideOutsideCache: Converting <" << symbol << "," << (int)begin << "," << (int)end << "> to 64bit key: " << (std::bitset<64>) buffer;
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

    
private:
    ProbabilisticContextFreeGrammar& grammar;

    CacheMap inside_cache;
    CacheMap outside_cache;
    
};

#endif	/* INSIDEOUTSIDECACHE_HPP */

