//
//  Signature.hpp
//  PCFG-EM
//
//  Created by Johannes Gontrum on 13/09/14.
//

#ifndef PCFG_EM_Signature_hpp
#define PCFG_EM_Signature_hpp

#include <boost/unordered_map.hpp>
#include <vector>
#include "easylogging++.h"

/// Maps objects to a unique numeric value.
template <typename EXTERNAL_OBJECT_TYPE>
class Signature {
    // Typedefs:
public:
    typedef EXTERNAL_OBJECT_TYPE Symbol;
    typedef int32_t ID;

private:
    typedef boost::unordered_map<Symbol, ID> SymbolToIDMap;
    typedef std::vector<Symbol> SymbolVector;



public:

    Signature() {
    }

    /// Returns true, if the given symbol does exist in the signature
    bool containsSymbol(const Symbol &symbol) const {
        return external_to_internal.find(symbol) != external_to_internal.end();
    }
    
    /// Returns true, if the given ID does exist in the signature
    bool containsID(const ID & id) const {
        return id < number_of_entries();
    }

    /// Adds a symbol and returns its ID. Can also be used to look-up a symbol.
    ID add_symbol(Symbol new_symbol) {
        ID result = resolve_symbol(new_symbol);
        if (result >= 0) { // the new symbol does already exist
            return result;
        } else {
            // add symbol to signature
            VLOG(7) << "Signature: New mapping added: '" << new_symbol << "' <-> " << number_of_entries();
            external_to_internal[new_symbol] = number_of_entries();
            internal_to_external.push_back(new_symbol);
            return number_of_entries() - 1;
        }
    }

    /*
     * Returns the ID for a symbol or a negative value (-1), if it does not exists.
     * Use add_symbol() as often as possible, if a non const method is ok.
     */
    ID resolve_symbol(const Symbol & symbol) const {
        return containsSymbol(symbol) ? external_to_internal.find(symbol)->second : -1; // return -1, if the symbol does not exist in the signature.
    }

    /// Returns the symbol for a given ID or an empty symbol
    Symbol resolve_id(ID const & unknown_id) const {
        static const Symbol NoSymbol;
        if (unknown_id >= number_of_entries() || unknown_id < 0) {
            return NoSymbol;
        }
        return internal_to_external[unknown_id];
    }

    friend std::ostream& operator<<(std::ostream& o, const Signature& signature) {
        o << "ID \t| Symbol\n----------------\n";
        for (unsigned i = 0; i < signature.number_of_entries(); ++i) {
            o << i << " \t| " << signature.resolve_id(i) << "\n";
        }
        return o;
    }


private:
    unsigned number_of_entries() const {
        return internal_to_external.size();
    }


private:
    SymbolVector internal_to_external; ///< Each index in the vector represents an external symbol.
    SymbolToIDMap external_to_internal; ///< Maps an external symbol to an integer.


};

#endif
