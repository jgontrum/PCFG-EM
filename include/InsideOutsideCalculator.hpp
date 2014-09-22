/* 
 * File:   InsideOutsideCalculator.hpp
 * Author: Johannes Gontrum
 *
 * Created on September 17, 2014, 6:51 PM
 */

#ifndef INSIDEOUTSIDECALCULATOR_HPP
#define	INSIDEOUTSIDECALCULATOR_HPP

#include "ProbabilisticContextFreeGrammar.hpp"
#include "Signature.hpp"
#include "PCFGRule.hpp"
#include "InsideOutsideCache.hpp"

#include <vector>
#include <string>
#include <cassert>

#include "easylogging++.h"

class InsideOutsideCalculator {
public:
    typedef PCFGRule::Probability                               Probability; 
    typedef ProbabilisticContextFreeGrammar::ExternalSymbol     ExternalSymbol;
    typedef std::string                                         String;
    typedef std::vector<ExternalSymbol>                         ExtVector;   
    typedef ProbabilisticContextFreeGrammar::Symbol             Symbol;
    typedef std::vector<Symbol>                                 SymbolVector;
    typedef ProbabilisticContextFreeGrammar::LHSRange           PCFGRange;
    typedef ProbabilisticContextFreeGrammar::const_iterator     PCFGCIt;
    
public:
    InsideOutsideCalculator(InsideOutsideCache& cache, ExtVector& sentence) :  grammar(cache.get_grammar()), signature(cache.get_grammar().get_signature()) {
        // create the input vector, that contains numeric values instead of strings 
        for (ExternalSymbol word : sentence) {
            input.push_back(signature.resolve_symbol(word));
        }
        for (Symbol sym : input) {
            std::cout << sym << " - ";
        }
        std::cout << "\n\n" << signature;
//        LOG(INFO) << "InsideOutsideCalculator: Created for  '" << *this << "' successfully created.";
    }
    
    /// Calculates the inside probability, that the given symbol produces a (part)
    /// of a sentence from a specified beginning position to an end position.
    /// See 'Foundations of Statistical Natural Language pProcessing' by Manning & Schuetze, pp.392 
    /// for further details about this implementation.
    Probability calculate_inside(Symbol symbol, unsigned short begin, unsigned short end) {
        LOG(INFO) << "InsideOutsideCalculator: Calculating Inside Probability: '" << signature.resolve_id(symbol) << "'(" << begin << ", " << end << ")";        PCFGRange rule_range = grammar.rules_for(symbol);
        assert(begin <= end);
        // Base case: The length of the span is 0
        if (begin == end) {
            // Check, that this is a valid request
            if (begin < input.size()) {
                Symbol terminal_symbol = input[begin];
                for (PCFGCIt rule = rule_range.first; rule != rule_range.second; ++rule) {
                    if ((*rule)[0] == terminal_symbol && rule->arity() == 1) return rule->get_prob();
                }
            } else return 0;
        }
        
        // Inductive case:
        Probability score = 0;
        // Iterate over all rules with two NTs on the rhs.
        for (PCFGCIt rule = rule_range.first; rule != rule_range.second; ++rule) { 
            if (rule->arity() == 2) {
                // Iterate over all possible divisions
                for (unsigned split = begin; split < end; ++split) {
                    // Multiply the probability of the rule with the inside probability
                    // of both parts.
                    double inner_score = rule->get_prob();
                    inner_score *= calculate_inside((*rule)[0], begin, split);
                    inner_score *= calculate_inside((*rule)[1], split +1, end);
                    // And add it to the overall score.
                    score += inner_score;
                }
            }
        }
        return score;
    }
    
    
    Probability calculate_outside(Symbol symbol, unsigned short length) {
        // Base case: The symbol is the start symbol and there is nothing outside the span
        if (length == input.size()) {
            return (grammar.get_start_symbol() == symbol)? 1 : 0;
        }
        
        // Inductive case:
        
        return 0;
    }
    
private:
    const ProbabilisticContextFreeGrammar&                        grammar;
    const ProbabilisticContextFreeGrammar::ExtSignature&          signature;
    SymbolVector                                            input;
};

#endif	/* INSIDEOUTSIDECALCULATOR_HPP */

