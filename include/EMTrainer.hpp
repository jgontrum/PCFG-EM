/* 
 * File:   EMTrainer.hpp
 * Author: Johannes Gontrum
 *
 * Created on September 17, 2014, 6:49 PM
 */

#ifndef EMTRAINER_HPP
#define	EMTRAINER_HPP

#include <iostream>
#include <unordered_map>

#include "ProbabilisticContextFreeGrammar.hpp"
#include "InsideOutsideCalculator.hpp"
#include "Signature.hpp"
#include "PCFGRule.hpp"

#include <boost/tokenizer.hpp>
#include <sstream>
#include <cmath>
#include <limits>       // std::numeric_limits

#include "../include/easylogging++.h"

/// Learns the probability distribution of a grammar based on raw, not annotated sentences.
class EMTrainer {
public:
    typedef ProbabilisticContextFreeGrammar::Probability    Probability;
private:
    typedef boost::char_separator<char>                     CharSeparator;
    typedef boost::tokenizer<CharSeparator>                 Tokenizer;
    typedef std::string                                     ExternalSymbol;
    typedef std::vector<ExternalSymbol>                     StringVector;
    typedef ProbabilisticContextFreeGrammar::Symbol         Symbol;
    typedef std::vector<Symbol>                             SymbolVector;
    typedef std::pair<SymbolVector, bool>                   SentenceTuple;
    typedef std::vector<SentenceTuple>                      SentencesVector;
    typedef std::unordered_map<Symbol, Probability>         SymbolToProbMap;
    typedef std::unordered_map<PCFGRule, Probability, PCFGRule::Hasher>       RuleToProbMap;

    
public:
    EMTrainer(ProbabilisticContextFreeGrammar& pcfg, std::istream& corpus) : 
    grammar(pcfg), signature(pcfg.get_signature()) {
        no_of_sentences = 0;
        read_in(corpus);
    }
    
    /// Perfom the EM training exactly x times.
    void train(unsigned no_of_loops) {
        bool cleaned = false;
        double last_changes = 0;
        
        for (unsigned i = 0; i < no_of_loops; ++i) {
            last_changes = train();
            
            // clean the grammar only after the first iteration.
            if (!cleaned) {
                grammar.clean_grammar();
                cleaned = true;
            }
        }

        VLOG(1) << "EMTrainer: Completed after " << no_of_loops << " iterations with change delta = " << last_changes << ".";

    }
    
    /// Perfom the EM training, until the changes are below the given threshold
    void train(double threshold) {
        double last_changes = std::numeric_limits<double>::max();
        bool cleaned = false;
        unsigned iterations = 0;
        
        while (last_changes > threshold) {
            ++iterations;
            last_changes = train();
            // clean the grammar only after the first iteration.
            if (!cleaned) {
                grammar.clean_grammar();
                cleaned = true;
            }
        }
        VLOG(1) << "EMTrainer: Completed " << iterations << " iterations until changes were " << last_changes << " (<= " << threshold << ").";
    }
    
    
private:
    double train() {
        SymbolToProbMap symbol_prob;
        RuleToProbMap rule_prob;
        bool training_performed = false;
        double delta = 0; // measure the change in probability

        // First, iterate over all sentences and sum up the estimations for the rules and the sentences themselves.
        VLOG(2) << "EMTrainer: Estimate probabilities for " << no_of_sentences << " sentences.";
        for (SentencesVector::const_iterator cit = sentences.begin(); cit != sentences.end(); ++cit) {
            if (cit->second != false) {
                training_performed = true; // in case there are no valid sentences in the training data
                InsideOutsideCache cache(grammar);
                InsideOutsideCalculator iocalc(cache, &(cit->first));

                unsigned len = (cit->first).size();
                VLOG(3) << "EMTrainer: Current sentence: '" << symbol_vector_to_string(cit->first) << "'";
                
                // Calculate the inside probabiliy for the whole sentence first.
                // in M&S this varible is called "Pi" and defined as 
                // P(w_1m | G) = P(N^1 =>* w_1m | G) = Beta_1(1,m)
                Probability inside_sentence = iocalc.calculate_inside(grammar.get_start_symbol(), 0, len-1);
                VLOG(4) << "EMTrainer: Inside Probability for the whole sentence is " << inside_sentence;
                
                // Estimate how many times a nonterminal was is in the current sentence
                for (Symbol nt : grammar.get_nonterminals()) {
                    symbol_prob[nt] += estimate_symbol_expectation(nt, len, inside_sentence, iocalc);
                }
                
                // Estimate how many times a rule is used.
                for (ProbabilisticContextFreeGrammar::iterator rule = grammar.begin(); rule != grammar.end(); ++rule) {
                    
                    if (rule->arity() == 2) { // Normal rules -> (11.26), p. 400                        
                        rule_prob[*rule] += estimate_rule_expectation((*rule), len, inside_sentence, iocalc);

                    } else { // Preterminal rules -> (11.27), p. 400
                        rule_prob[*rule] += estimate_terminal_rule_expectation((*rule), len, cit->first, inside_sentence, iocalc) ;
                    }

                }
            }
        }
        if (training_performed) {
            // Now that all sentences have been processed, it is time for the maximisation step:
            // Maximize the probability of the rules in the grammar
            VLOG(2) << "EMTrainer: Maximize the probabilities of all rules in the grammar.";
            for (ProbabilisticContextFreeGrammar::iterator rule = grammar.begin(); rule != grammar.end(); ++rule) {
                assert(symbol_prob.find(rule->get_lhs()) != symbol_prob.end());

                Probability summed_sentence_estimation = symbol_prob[rule->get_lhs()];
                Probability new_prob;
                // Divide the summed up estimation for all rules by the summed up estimation of the symbol on the lhs.
                if (summed_sentence_estimation > 0) { // avoid division by 0
                    new_prob = rule_prob[*rule] / summed_sentence_estimation;
                    delta += std::abs(rule->get_prob() - new_prob);
                } else {
                    new_prob = 0;
                }
                VLOG(9) << "EMTrainer: Updating probability for rule '" << *rule << "'. New: " << new_prob;
                rule->set_probability(new_prob);
            }

//            assert(grammar.is_valid_pcfg());
        } else {
            LOG(WARNING) << "EMTrainer: No estimation or maximization step performed. Please check, if the sentences in the training data can be parsed with the given grammar.";
        }
        
        VLOG(2) << "EMTrainer: Changes made in this iteration: " << delta;
        return delta;

    }

    /// This function is an implementation of fig. (11.24) on p. 399 in Manning&Schuetze.
    /// It calculates the estimate for how many times a NT is used in the derivation of the current sentence.
    Probability estimate_symbol_expectation(const Symbol& symbol, unsigned len, Probability pi, InsideOutsideCalculator& iocalc) {
        // Iterate over all possible ranges in the sentence
        Probability score = 0;
        for (unsigned p = 0; p < len; ++p) {
            for (unsigned q = p; q < len; ++q) {
                Probability current_outside = iocalc.calculate_outside(symbol, p, q);
                Probability current_inside = iocalc.calculate_inside(symbol, p, q);
                
                Probability current_result = (current_outside * current_inside) / pi;
                score += current_result;
            }
        }
        VLOG(6) << "EMTrainer: Estimation for the symbol '" << signature.resolve_id(symbol) << "' is "<<score;
        return score;
    }
    
    /// Like estimate_symbol_expectationm, but for rules. See fig. (11.25) on p. 400 in Manning&Schuetze.
    /// Again, we do not divide the result by the inside probability of the whole sentence.
    Probability estimate_rule_expectation(const PCFGRule& rule, unsigned len, Probability pi, InsideOutsideCalculator& iocalc) {
        assert(rule.arity() == 2);
        Probability score = 0;
        
        for (unsigned p = 0; p < len-1; ++p) {
            for (unsigned q = p+1; q < len; ++q) {
                Probability inner_score = 0;
                for (unsigned d = p; d < q; ++d) {
                    Probability outside_lhs = iocalc.calculate_outside(rule.get_lhs(), p, q);
                    Probability inside_rhs1 = iocalc.calculate_inside(rule[0], p, d);
                    Probability inside_rhs2 = iocalc.calculate_inside(rule[1], d+1, q);
                    
                    Probability current_score = rule.get_prob() * outside_lhs * inside_rhs1 * inside_rhs2;
                    
                    inner_score += current_score;
                }
                score += inner_score / pi;
            }
        }
        VLOG(6) << "EMTrainer: Estimation for the rule '" << rule << "': " << score;
        return score;
    }
    
    /// Like estimate_symbol_expectationm but for terminal rules. 
    /// See Manning&Schuetze: p.400, (11.27). This function implements the numerator of the fraction,
    /// as the denumerator has been calculated in advance.
    Probability estimate_terminal_rule_expectation(const PCFGRule& rule, unsigned len,  const SymbolVector& sentence, Probability pi, InsideOutsideCalculator& iocalc) {
        assert(rule.arity() == 1);
        
        Probability score = 0;
        
        for (unsigned h = 0; h < len; ++h) {
            // P(w_h = w^k) - check, if the terminal in the sentence at position h and the the terminal on the rhs are the same.
            if (rule.get_rhs()[0] == sentence[h]) {
                
                Probability outside = iocalc.calculate_outside(rule.get_lhs(), h, h);
                Probability inside = iocalc.calculate_inside(rule.get_lhs(), h, h);
                                
                score += (outside * inside) / pi;
                
            } // else: add 0 to the score.
        }

        VLOG(6) << "EMTrainer: Estimation for the rule '" << rule << "': " << score;
        return score;
    }

    void read_in(std::istream& corpus) {
        std::string line;
        unsigned line_no = 1;
        VLOG(4) << "EMTrainer: Reading in the training corpus...";
        while (corpus.good()) {
            std::getline(corpus, line);
            if (!line.empty()) {
                VLOG(6) << "EMTrainer: Reading in line " << line_no << ": '" << line << "'.";
                // Tokenize line
                Tokenizer tokens(line, CharSeparator("\t "));
                SymbolVector tokens_id;;
                bool valid = true;
                
                for (ExternalSymbol word : tokens) {
                    Symbol word_as_id = signature.resolve_symbol(word);
                    if (word_as_id < 0) { // If a terminal symbol was not found, mark this sentence as invalid.
                        LOG(ERROR) << "EMTrainer: Sentence in line " << line_no << " will be ignored, the token '" << word<< "' cannot be resolved.";
                        valid = false;
                    }
                    tokens_id.push_back(word_as_id);
                }
                
                ++no_of_sentences;
                sentences.push_back(SentenceTuple(tokens_id, valid));
            }
            ++line_no;
        }
    }
    
    /// Nice way to print a symbol vector (sentences)
    std::string symbol_vector_to_string(const SymbolVector& vector) {
        std::stringstream sstream;
        
        for (Symbol sym : vector) {
            sstream << signature.resolve_id(sym) << " ";
        }
        
        return sstream.str();
    }
    
private:
    ProbabilisticContextFreeGrammar& grammar; ///< the grammar (obvious)
    Signature<ExternalSymbol>& signature; ///< the signature
    unsigned no_of_sentences; ///< the number of sentences in the corpus
    SentencesVector sentences; ///< a vector of the sentences in the training corpus
    
};

#endif	/* EMTRAINER_HPP */

