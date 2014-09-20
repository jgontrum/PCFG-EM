////////////////////////////////////////////////////////////////////////////////
// ProbabilisticContextFreeGrammar.hpp
// Class representing a probabilistic context free grammar.
// Johannes Gontrum, 17.9.2014 (based upon TH, 16.6.2014)
////////////////////////////////////////////////////////////////////////////////

#ifndef __PROBABILISTICCONTEXTFREEGRAMMAR_HPP__
#define __PROBABILISTICCONTEXTFREEGRAMMAR_HPP__

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include <boost/tokenizer.hpp>
#include <boost/unordered_set.hpp>

#include "PCFGRule.hpp"
#include "Signature.hpp"

#include "easylogging++.h"

class ProbabilisticContextFreeGrammar {
public:
    typedef PCFGRule::ID                                Symbol;
    typedef boost::unordered_set<Symbol>                SymbolSet;
    typedef PCFGRule::ExternalSymbol                    ExternalSymbol;
    typedef Signature<ExternalSymbol>                   ExtSignature;

private:
    typedef SymbolSet::const_iterator                   SymbolSetIter;
    typedef std::vector<PCFGRule>                       RuleVector;

public: 
    // Rule iterator
    typedef RuleVector::const_iterator                  const_iterator;
    typedef RuleVector::iterator                        iterator;
    typedef std::pair<const_iterator, const_iterator>   LHSRange;
    typedef std::pair<iterator, iterator>               MutableLHSRange;


private:
    typedef std::map<Symbol, LHSRange> RuleIndex;

public: // Functions  

    /// Constructs this grammar by reading a grammar from a stream.
    /// The given grammar must contain one PCFG per line (S --> NP VP [1.0]),
    /// while the first seen symbol defines the start symbol.
    ProbabilisticContextFreeGrammar(std::istream& grm_in) {
        read_in(grm_in);
    }

    /// Returns the id of the start symbol. Use the signature to translate it
    /// into a string.
    const Symbol& get_start_symbol() const {
        return start_symbol;
    }
    
    /// Get the Signature, that is used in this grammar. Other classes are allowed to
    /// add new symbols.
    ExtSignature& get_signature() {
        return signature;
    }
    
    const SymbolSet& get_nonterminals() {
        return nonterminal_symbols;
    }

    /// True, if the given symbol is a non-terminal.
    bool is_nonterminal(const Symbol& sym) const {
        return nonterminal_symbols.find(sym) != nonterminal_symbols.end();
    }

    /// True, if the given symbol is a terminal.
    bool is_terminal(const Symbol& sym) const {
        return nonterminal_symbols.find(sym) == nonterminal_symbols.end();
    }

    LHSRange rules_for(const Symbol& lhs) const {
        // Anm. LHSRange ist ein ziemlich einfaches Objekt, so dass man es auch
        // per Wert (also nicht per Referenz zur�ckgeben kann)
        RuleIndex::const_iterator f_lhs = rule_index.find(lhs);
        return (f_lhs != rule_index.end()) ? f_lhs->second : LHSRange(end(), end());
    }


    const_iterator begin() const {
        return productions.begin();
    }

    const_iterator end() const {
        return productions.end();
    }

    /// True, if this grammar is in Chompsky-Normal Form
    bool is_in_cnf() const {
        // iterate over all rules...
        for (const_iterator r = begin(); r != end(); ++r) {
            const PCFGRule& rule = *r;
            if (rule.arity() == 1) {
                // If it is a unary rule, the rhs must be terminal symbol.
                if (is_nonterminal(rule[0]))
                    return false;
            } else if (rule.arity() == 2) {
                // If it is a binary rule, both children mist be nonterminals.
                if (!is_nonterminal(rule[0]) || !is_nonterminal(rule[1]))
                    return false;
            } else return false;
        } 
        return true;
    }

    /// Returns true if the probabilities for all rules with the same lhs-symbol sum up to 1.
    bool is_valid_pcfg() const {
        for (Symbol lhs : nonterminal_symbols) {
            double score = 0.0;
            LHSRange range = rules_for(lhs);
            // iterate over all rules...
            for (const_iterator rule = range.first; rule != range.second; ++rule) {
                score += rule->get_prob();
            }
            // leave, if the score is not exactly 1
            if (score != 1) return false;
        }
        return true;
    }
    
    /// Stream output operator
    friend std::ostream& operator<<(std::ostream& o, const ProbabilisticContextFreeGrammar& g) {
        g.print(o);
        return o;
    }

private: 
    /// Read in the grammar from a stream.
    bool read_in(std::istream& grm_in) {
        std::string line;
        unsigned line_no = 1;
        bool first_rule = true;

        while (grm_in.good()) {
            std::getline(grm_in, line);
            if (!line.empty() && line[0] != '#') {
                /// create a rule
                PCFGRule r(line, signature);
                if (r) {
                    add_rule(r);
                    if (first_rule) {
                        // Set the start symbol as the first symbol that we encounter.
                        set_start_symbol(r.get_lhs());
                        first_rule = false;
                    }
                } else {
                    LOG(WARNING) << "PCFG: Rule in line " << line_no << " is ignored.";
                }
            }
            ++line_no;
        }

        // Sort the rules using the '<'-operator of PCFGRule
        std::sort(productions.begin(), productions.end());
        // build the index 
        build_rule_index();
        return true;
    }

    
    // Add a rule to the rule-vector
    void add_rule(const PCFGRule& r) {
        // save it in the rule vector
        productions.push_back(r);
        
        // add the lhs to the nonterminals and the vocabulary
        nonterminal_symbols.insert(r.get_lhs());
        vocabulary.insert(r.get_lhs());
        
        // add all symbols from the rhs to the nonterminals.
        vocabulary.insert(r.get_rhs().begin(), r.get_rhs().end());
    }

    /// Define the start symbol by a given symbol ID
    void set_start_symbol(const Symbol& start) {
        if (signature.containsID(start)) {
            start_symbol = start;
            nonterminal_symbols.insert(start);
        } else {
            LOG(WARNING) << "PCFG: The start symbol could not be set, because it is illegal. "
                    "It must be contained in the signature of the grammar.";
        }
        
    }

 
    void build_rule_index() {
        if (!productions.empty()) {
            // Erstes LHS-Symbol im Regelvektor bestimmen
            Symbol current_lhs = begin()->get_lhs();
            // Anfang des Bereichs f�r dieses Symbol
            const_iterator left = begin();
            for (const_iterator r = begin(); r != end(); ++r) {
                if (r->get_lhs() != current_lhs) {
                    // Nichtterminalbereich ist hier zu Ende
                    rule_index[current_lhs] = LHSRange(left, r);
                    // current_lhs ist nun das neue LHS-Symbol
                    current_lhs = r->get_lhs();
                    // r wird nun zum Anfang des neuen Bereichs
                    left = r;
                }
            } // for
            // Der letzte Bereich muss noch eingetragen werden
            rule_index[current_lhs] = LHSRange(left, end());
        }
    }

    /// Print the grammar to a given stream
    void print(std::ostream& o) const {
        // startsymbol
        o << signature.resolve_id(start_symbol) << "\n";

        // rules
        for (const_iterator r = begin(); r != end(); ++r) {
            o << *r << "\n";
        }
    }

    void print_symbol_set(std::ostream& o, const SymbolSet& syms) const {
        o << "{";
        for (SymbolSetIter s = syms.begin(); s != syms.end();) {
            o << *s;
            if (++s != syms.end()) o << ",";
            else break; // Kein , nach letztem Element
        }
        o << "}";
    }


private: // Instanzvariablen
    Symbol start_symbol; ///< Startsymbol
    SymbolSet nonterminal_symbols; ///< Nichtterminale
    SymbolSet vocabulary; ///< Terminale U Nichtterminale
    RuleVector productions; ///< Produktionsregeln
    RuleIndex rule_index; ///< Indexstruktur zum Auffinden von Regeln
    ExtSignature signature; ///< The signature to translate the symbol IDs of the rules to strings
}; // ContextFreeGrammar

#endif
