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
#include <unordered_map>

#include "PCFGRule.hpp"
#include "Signature.hpp"

#include "easylogging++.h"

/// Represents a PCFG with a signature.
class ProbabilisticContextFreeGrammar {
public:
    typedef PCFGRule::ID                                Symbol;
    typedef boost::unordered_set<Symbol>                SymbolSet;
    typedef PCFGRule::ExternalSymbol                    ExternalSymbol;
    typedef Signature<ExternalSymbol>                   ExtSignature;
    typedef std::vector<const PCFGRule*>                RulePointerVector;
    typedef PCFGRule::Probability                       Probability;

private:
    typedef SymbolSet::const_iterator                          SymbolSetIter;
    typedef std::vector<PCFGRule>                              RuleVector;
    typedef std::unordered_map<Symbol, RulePointerVector>      SymbolToRuleVectorMap;
    typedef std::unordered_map<Symbol, unsigned>               SymbolToCounterMap;
    typedef std::unordered_map<Symbol, Probability>            SymbolToProbabilityMap;


public: 
    // Rule iterator
    typedef RuleVector::const_iterator                  const_iterator;
    typedef RuleVector::iterator                        iterator;
    typedef std::pair<iterator, iterator>               LHSRangeMutable;
    typedef std::pair<const_iterator, const_iterator>   LHSRange;


private:
    typedef std::map<Symbol, LHSRangeMutable> RuleIndex;

public: // Functions  

    /// Constructs this grammar by reading a grammar from a stream.
    /// The given grammar must contain one PCFG per line (S --> NP VP [1.0]),
    /// while the first seen symbol defines the start symbol.
    ProbabilisticContextFreeGrammar(std::istream& grm_in) {
        read_in(grm_in);
        build_rule_rhs_index();
        normalize_probabilities();
    }

    /// Returns the id of the start symbol. Use the signature to translate it
    /// into a string.
    const Symbol& get_start_symbol() const {
        return start_symbol;
    }
    
    /// Get the Signature, that is used in this grammar. Other classes are allowed to
    /// add new symbols.
    const ExtSignature& get_signature() const {
        return signature;
    }

    /// Like the const version, but here you can alter the signature
    ExtSignature& get_signature() {
        return signature;
    }
    
    /// Return a set of all nonterminals
    const SymbolSet& get_nonterminals() const {
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

    /// Returns a range of rules for a given lhs symbol
    LHSRange rules_for(const Symbol& lhs) const {
        RuleIndex::const_iterator f_lhs = rule_index.find(lhs);
        return (f_lhs != rule_index.end()) ? f_lhs->second : LHSRange(end(), end());
    }
    
    /// Returns a range of rules (that can be changed) for a given lhs symbol
    LHSRangeMutable rules_for(const Symbol& lhs)  {
        RuleIndex::iterator f_lhs = rule_index.find(lhs);
        return (f_lhs != rule_index.end()) ? f_lhs->second : LHSRangeMutable(end(), end());
    }

    /*
     * Returns a const vector of const pointers to rules, that have the given symbol as the first symbol on their rhs.
     * Example: [A -> B C] and [X -> B Y] will be returned when this method is called with 'B' as an argument.
     * This function and its sister function are useful while computing an EM-Algorithm.
     */
    inline const RulePointerVector * const get_rules_for_first_symbol(const Symbol& first_symbol) const {
        SymbolToRuleVectorMap::const_iterator cit = first_symbol_rules.find(first_symbol);
        return cit != first_symbol_rules.end() ? &(cit->second) : nullptr;
    }

    /*
     * Returns a const vector of const pointers to rules, that have the given symbol as the second symbol on their rhs.
     * Example: [A -> C B] and [X -> Y B] will be returned when this method is called with 'B' as an argument.
     * This function and its sister function are useful while computing an EM-Algorithm.
     */
    inline const RulePointerVector * const get_rules_for_second_symbol(const Symbol& second_symbol) const {
        SymbolToRuleVectorMap::const_iterator cit = second_symbol_rules.find(second_symbol);
        return cit != second_symbol_rules.end() ? &(cit->second) : nullptr;
    }

    
    iterator begin() {
        return productions.begin();
    }

    iterator end() {
        return productions.end();
    }

    const_iterator begin() const {
        return productions.begin();
    }

    const_iterator end() const {
        return productions.end();
    }

    /// True, if this grammar is in Chomsky-Normal Form
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
    

    
    /*
     * Removes all rules, that have probability=0
     */
    void clean_grammar() {
        VLOG(4) << "PCFG: Cleaning - Starting cleaning process...";
        VLOG(5) << "PCFG: Cleaning - Currently there are " << productions.size() << " rules in this grammar.";

        unsigned no_rules_before_clean = productions.size();

        // sort by their probability, so we have all rules with prob=0 right next to each other
        std::sort(begin(), end(), PCFGRule::compare_by_probability());


        // now find the range of rules with prob zero
        const_iterator begin_of_zeros;
        const_iterator end_of_zeros;
        bool in_range = false;

        for (const_iterator rule = begin(); rule != end(); ++rule) {
            //            std::cout << *rule << "\n";
            if (rule->get_prob() == 0) {
                if (!in_range) {
                    in_range = true;
                    begin_of_zeros = rule;
                }
            } else {
                if (in_range) {
                    end_of_zeros = rule;
                    break; // no need to continue, we have found all the rules we want to remove
                }
            }
        }

        // finally, remove all zero rules!
        productions.erase(begin_of_zeros, end_of_zeros);

        // sort in the right order again
        std::sort(begin(), end());

        // clear variables;
        rule_index.clear();
        first_symbol_rules.clear();
        second_symbol_rules.clear();
        nonterminal_symbols.clear();
        vocabulary.clear();

        // Rebuild structures if anything was changed.
        VLOG(6) << "PCFG: Cleaning - Rebuilding the rule index...";
        build_rule_index();
        VLOG(6) << "PCFG: Cleaning - Finished rebuilding the rule index!";

        VLOG(6) << "PCFG: Cleaning - Rebuilding rhs vectors...";
        build_rule_rhs_index();
        VLOG(6) << "PCFG: Cleaning - Finished rebuilding rhs vectors!";


        assert(nonterminal_symbols.find(get_start_symbol()) != nonterminal_symbols.end());

        VLOG(4) << "PCFG: Cleaning - Finished cleaning process! " << no_rules_before_clean - productions.size() << " rules have been deleted!";

    }

    
    /*
     * Checks, if this grammar is a valid PCFG, so that the probabilities of all rules
     * that share the same lhs-symbol sum up to one.
     * Is that not the case, the rules will receive the probability 
     * P(rule) / sum(P(rules)) [for all rules with the same lhs symbol].
     */
    void normalize_probabilities() {
        // Iterate over all rules and sum up the probability for all rules for a lhs symbol.
        // Also count, how many rules belong to a lhs symbol.
        
        // Iterate over all nonterminals...
        for (const Symbol& nt : get_nonterminals()) {
            Probability current_probability = 0.0;
            unsigned counter = 0;            
            // ... and their rules.
            LHSRangeMutable rules_for_nt = rules_for(nt);
            for (iterator rule = rules_for_nt.first; rule != rules_for_nt.second; ++rule) { 
                ++counter;
                current_probability += rule->get_prob();
            }
                        
            // If the summed up probability is not exactly 1, normalize the probability for all 
            // rules for this lhs symbol. We assign them the probability p / current_probability.
            if ((int)(current_probability*1000000+0.5)/1000000.0 != 1) { // ceil
                LOG(WARNING) << "PCFG: Probabilities for the symbol '" << get_signature().resolve_id(nt) << "' sum up to '" << current_probability << "' and are therefore illegal. Belonging rules will be normalized.";;
                for (iterator rule = rules_for_nt.first; rule != rules_for_nt.second; ++rule) {
                    rule->set_probability(rule->get_prob()/current_probability);
                }
            }
        }
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
                if (first_rule) { // this is the first line of the grammar 
                    first_rule = false;
                    VLOG(5) << "PCFG: Setting '" << line << "' as startsymbol.";
                    set_start_symbol(signature.add_symbol(line));
                    continue;
                } else {
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
    void add_rule(PCFGRule& r) {
        // save it in the rule vector
        productions.push_back(r);
        RuleVector::iterator it = productions.begin();
    }

    /// Define the start symbol by a given symbol ID
    void set_start_symbol(const Symbol& start) {
        if (signature.containsID(start)) {
            start_symbol = start;
        } else {
            LOG(WARNING) << "PCFG: The start symbol could not be set, because it is illegal. "
                    "It must be contained in the signature of the grammar.";
        }
        
    }

 
    void build_rule_index() {
        if (!productions.empty()) {
            Symbol current_lhs = begin()->get_lhs();
            iterator left = begin();
            for (iterator r = begin(); r != end(); ++r) {
                // Insert nonterminals and vocabulary
                nonterminal_symbols.insert(r->get_lhs());
                vocabulary.insert(r->get_lhs());
                vocabulary.insert(r->get_rhs().begin(), r->get_rhs().end());
                
                
                if (r->get_lhs() != current_lhs) {

                    rule_index[current_lhs] = LHSRangeMutable(left, r);

                    current_lhs = r->get_lhs();

                    left = r;
                }
            }

            rule_index[current_lhs] = LHSRangeMutable(left, end());
        }
    }
    
    
    void build_rule_rhs_index() {
        // iterate over all non terminals...
        for (const Symbol& nt : get_nonterminals()) {
            // ... and their rules.
            LHSRange rules_for_nt = rules_for(nt);
            for (const_iterator rule = rules_for_nt.first; rule != rules_for_nt.second; ++rule) {
                if (rule->arity() == 2) {
                    // map the first and the second symbol on the rhs to a vector containing a pointer to this rule
                    first_symbol_rules[(*rule)[0]].push_back(&(*rule));
                    second_symbol_rules[(*rule)[1]].push_back(&(*rule));
                }
            }
        }
    }
   
    /// Print the grammar to a given stream
    void print(std::ostream& o) const {
        // startsymbol
        o << signature.resolve_id(start_symbol) << "\n";

        // rules
        for (const_iterator r = begin(); r != end(); ++r) {
            if (r->get_prob() > 0) {
                o << *r << "\n";

            }
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


private:
    Symbol start_symbol; ///< startsymbol
    SymbolSet nonterminal_symbols; ///< nonterminals
    SymbolSet vocabulary; ///< terminals and nonterminals
    RuleVector productions; ///< rules
    RuleIndex rule_index; ///< index of the rules
    ExtSignature signature; ///< The signature to translate the symbol IDs of the rules to strings

    SymbolToRuleVectorMap first_symbol_rules; ///< Maps a symbol to all rules, where it appeares as the first symbol on the rhs.
    SymbolToRuleVectorMap second_symbol_rules; ///< Maps a symbol to all rules, where it appeares as the second symbol on the rhs.
}; 

#endif
