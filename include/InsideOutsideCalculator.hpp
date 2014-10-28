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

/// Calculates inside and outside values for a sentence.
class InsideOutsideCalculator {
public:
    typedef InsideOutsideCache::InsideOutsideProbability        InsideOutsideProbability;
    typedef ProbabilisticContextFreeGrammar::Symbol             Symbol;
    typedef InsideOutsideCache::LengthType                      LengthType;

 
private:
    typedef std::string String;
    typedef ProbabilisticContextFreeGrammar::RulePointerVector  RulePointerVector;
    typedef std::vector<Symbol>                                 SymbolVector;
    typedef ProbabilisticContextFreeGrammar::LHSRange           PCFGRange;
    typedef ProbabilisticContextFreeGrammar::const_iterator     PCFGCIt;
    
public:
    InsideOutsideCalculator(InsideOutsideCache& iocache)
    :
    grammar(iocache.get_grammar()),
    signature(iocache.get_grammar().get_signature()),
    cache(iocache),
    sentence_len(0) {
        input = nullptr;
    }

    /// Sets the new sentence. You have to do this before starting a calculation!
    void set_sentence(const SymbolVector * new_sentence) {
        input = new_sentence;
        if (new_sentence != nullptr) {
            sentence_len = new_sentence->size();
        }
    }
    
    /*
     *  Calculates the inside probability, that the given symbol produces a (part)
     *  of a sentence from a specified beginning position to an end position.
     *  See 'Foundations of Statistical Natural Language Processing' by Manning & Schuetze, pp.392 
     *  for further details about this implementation.
    */ 
    InsideOutsideProbability calculate_inside(const Symbol& symbol, const LengthType& begin, const LengthType& end) {
        VLOG(7) << "InsideOutsideCalculator: Calculating Inside Probability: '" << signature.resolve_id(symbol) << "'(" << (unsigned)begin << ", " << (unsigned)end << ")";
        if (input != nullptr) {
            assert(begin <= end);
            assert(begin < sentence_len);
            assert(end < sentence_len);

            PCFGRange rule_range = grammar.rules_for(symbol);

            // First, check if we have already calculated this value
            const InsideOutsideProbability * const cached_prob = cache.get_inside_cache(symbol, begin, end);
            if (cached_prob != nullptr) {
                VLOG(8) << "InsideOutsideCalculator: Using cached Inside Probability (" << *cached_prob << ") for '" << signature.resolve_id(symbol) << "'(" << (unsigned) begin << ", " << (unsigned) end << ")";

                return *cached_prob;
            }

            // Base case: The length of the span is 0
            if (begin == end) {
                // Check, that this is a valid request
                if (begin < sentence_len) {
                    Symbol terminal_symbol = (*input)[begin];
                    for (PCFGCIt rule = rule_range.first; rule != rule_range.second; ++rule) {
                        if ((*rule)[0] == terminal_symbol && rule->arity() == 1) {
                            VLOG(8) << "InsideOutsideCalculator: Inside Probability: is '" << rule->get_prob() << "' for " << signature.resolve_id(symbol) << "'(" << (unsigned) begin << ", " << (unsigned) end << ")";

                            cache.store_inside_cache(symbol, begin, end, rule->get_prob());
                            return rule->get_prob();
                        }
                    }
                } else {
                    VLOG(8) << "InsideOutsideCalculator: Inside Probability: is '0' for " << signature.resolve_id(symbol) << "'(" << (unsigned) begin << ", " << (unsigned) end << ")";

                    cache.store_inside_cache(symbol, begin, end, 0);
                    return 0;
                }
            }

            // Inductive case:
            InsideOutsideProbability score = 0;
            // Iterate over all rules with two NTs on the rhs.
            for (PCFGCIt rule = rule_range.first; rule != rule_range.second; ++rule) {
                if (rule->arity() == 2) {
                    // Iterate over all possible divisions
                    for (unsigned split = begin; split < end; ++split) {
                        // Multiply the probability of the rule with the inside probability
                        // of both parts.
                        double inner_score = rule->get_prob();
                        inner_score *= calculate_inside((*rule)[0], begin, split);
                        inner_score *= calculate_inside((*rule)[1], split + 1, end);
                        // And add it to the overall score.
                        score += inner_score;
                    }
                }
            }
            VLOG(7) << "InsideOutsideCalculator: Inside Probability: is '" << score << "' for " << signature.resolve_id(symbol) << "'(" << (unsigned) begin << ", " << (unsigned) end << ")";

            cache.store_inside_cache(symbol, begin, end, score);
            return score;
        } else {
            LOG(ERROR) << "InsideOutsideCalculator: Cannot calculate Inside Probability, set the sentence first!";
            return 0;
        }
    }

    /*
     *  Calculates the outside probability of a given symbol and a number of symbols to the
     *  left and to the right.
     *  See 'Foundations of Statistical Natural Language Processing' by Manning & Schuetze, pp.400 
     *  for further details about this implementation.
     */ 
    InsideOutsideProbability calculate_outside(const Symbol& symbol, const LengthType& left, const LengthType& right) {
        VLOG(7) << "InsideOutsideCalculator: Calculating Outside Probability: '" << signature.resolve_id(symbol) << "'(" << (unsigned) left << ", " << (unsigned) right << ")";
        if (input != nullptr) {
            assert(left <= right);
            assert(left < sentence_len);
            assert(right < sentence_len);

            // Check the cache first
            const InsideOutsideProbability * const cached_prob = cache.get_outside_cache(symbol, left, right);
            if (cached_prob != nullptr) {
                VLOG(8) << "InsideOutsideCalculator: Using cached Outside Probability (" << *cached_prob << ") for '" << signature.resolve_id(symbol) << "'(" << (unsigned) left << ", " << (unsigned) right << ")";
                return *cached_prob;
            }

            // Base case: begin is 0 and end the the length of the sentence (-1)
            if (left == 0 && right == sentence_len - 1) {
                // If the symbol is the start symbol, it covers the whole sentence, so we return 1
                if (grammar.get_start_symbol() == symbol) {
                    VLOG(8) << "InsideOutsideCalculator: Outside probability  is '1' for '" << signature.resolve_id(symbol) << "'(" << (unsigned) left << ", " << (unsigned) right << ")";

                    cache.store_outside_cache(symbol, left, right, 1);
                    return 1;
                } else { // If not, this case is not possible and we return 0
                    VLOG(8) << "InsideOutsideCalculator: Outside probability  is '0' for '" << signature.resolve_id(symbol) << "'(" << (unsigned) left << ", " << (unsigned) right << ")";

                    cache.store_outside_cache(symbol, left, right, 0);
                    return 0;
                }
            }

            // Inductive case:
            // Case 1: 'symbol' is the left Symbol on the rhs of a rule.
            InsideOutsideProbability score_left = 0;
            // Iterate over all rules, where 'symbol' is the first symbol on the rhs:
            // Remember: RulePointerVector is a a vector of pointers, so dereferencing 
            // the iterator 'rule' will give you the pointer, not the _actual_ object.
            const RulePointerVector * rules = grammar.get_rules_for_first_symbol(symbol);
            if (rules != nullptr) {
                for (RulePointerVector::const_iterator rule = rules->begin();
                        rule != rules->end();
                        ++rule) {
                    VLOG(8) << "InsideOutsideCalculator: Current rule: '" << **rule << "' with '" << signature.resolve_id(symbol) << "' as first symbol on the rhs.'";

                    assert((*rule)->arity() == 2);

                    // Iterate over all possible divisions
                    for (unsigned split = right + 1; split < sentence_len; ++split) {
                        InsideOutsideProbability inner_score = calculate_outside((*rule)->get_lhs(), left, split); // Recursion
                        inner_score *= (*rule)->get_prob();
                        inner_score *= calculate_inside((**rule)[1], right + 1, split); // inside for the other child symbol

                        score_left += inner_score;
                    }
                }
            } else {
                VLOG(8) << "InsideOutsideCalculator: No rule with '" << signature.resolve_id(symbol) << "' as first symbol on the rhs exists.'";
            }


            VLOG(8) << "InsideOutsideCalculator: Outside probability for the left child is '" << score_left << "' for '" << signature.resolve_id(symbol) << "'(" << (unsigned) left << ", " << (unsigned) right << ")";


            // Case 2: 'symbol' is the right Symbol on the rhs of a rule.
            InsideOutsideProbability score_right = 0;
            // Iterate over all rules, where 'symbol' is the second symbol on the rhs:
            rules = grammar.get_rules_for_second_symbol(symbol);
            if (rules != nullptr) {
                for (RulePointerVector::const_iterator rule = rules->begin();
                        rule != rules->end();
                        ++rule) {
                    VLOG(8) << "InsideOutsideCalculator: Current rule: '" << **rule << "' with '" << signature.resolve_id(symbol) << "' as second symbol on the rhs.'";

                    assert((*rule)->arity() == 2);
                    // Iterate over all possible divisions
                    for (unsigned split = 0; split < left; ++split) {
                        InsideOutsideProbability inner_score = calculate_outside((*rule)->get_lhs(), split, right); // Recursion
                        inner_score *= (*rule)->get_prob();
                        inner_score *= calculate_inside((**rule)[0], split, left - 1); // inside for the other child symbol

                        score_left += inner_score;
                    }
                }
            } else {
                VLOG(8) << "InsideOutsideCalculator: No rule with '" << signature.resolve_id(symbol) << "' as second symbol on the rhs exists.'";
            }

            VLOG(8) << "InsideOutsideCalculator: Outside probability for the right child is '" << score_right << "' for '" << signature.resolve_id(symbol) << "'(" << (unsigned) left << ", " << (unsigned) right << ")";

            VLOG(7) << "InsideOutsideCalculator: Outside probability  is '" << score_left + score_right << "' for '" << signature.resolve_id(symbol) << "'(" << (unsigned) left << ", " << (unsigned) right << ")";

            cache.store_outside_cache(symbol, left, right, score_left + score_right);

            return score_left + score_right;
        } else {
            LOG(ERROR) << "InsideOutsideCalculator: Cannot calculate Outside Probability, set the sentence first!";
            return 0;
        }
    }
    
private: 
    const ProbabilisticContextFreeGrammar&                        grammar;      ///< Gramamr to lookup the rules
    const ProbabilisticContextFreeGrammar::ExtSignature&          signature;    ///< Signarue for prettier verbose messages
    const SymbolVector *                                          input;        ///< The current sentence
    LengthType                                                    sentence_len; ///< The length of the current sentence
    InsideOutsideCache&                                           cache;        ///< A cache to store all calculated values
};

#endif	/* INSIDEOUTSIDECALCULATOR_HPP */

