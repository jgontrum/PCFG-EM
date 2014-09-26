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

#include "../include/easylogging++.h"

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
        iocalc = nullptr;
        no_of_sentences = 0;
        read_in(corpus);
        train();
    }
    
    void train(unsigned no_of_loops) {
        for (unsigned i = 0; i < no_of_loops; ++i) {
            train();
        }
    }
    
    ProbabilisticContextFreeGrammar& get_grammar() {
        return grammar;
    }
    

    
private:
    void train() {

        
        for (SentencesVector::const_iterator cit = sentences.begin(); cit != sentences.end(); ++cit) {
            if (cit->second != false) {
                InsideOutsideCache cache(grammar); // needs to be rebuild after changing the grammar
                InsideOutsideCalculator iocalc(cache);
                
                SymbolToProbMap symbol_prob;
                RuleToProbMap rule_prob;
                
                iocalc.set_sentence(&(cit->first));
                unsigned len = (cit->first).size();
                LOG(INFO) << "EMTrainer: Current sentence: '" << symbol_vector_to_string(cit->first) << "'";
                
                // Calculate the inside probabiliy for the whole sentence first.
                // in M&S this varible is called "Pi" and defined as 
                // P(w_1m | G) = P(N^1 =>* w_1m | G) = Beta_1(1,m)
                Probability inside_sentence = iocalc.calculate_inside(grammar.get_start_symbol(), 0, len-1);
                LOG(INFO) << "EMTrainer: Inside Probability for the whole sentence is " << inside_sentence;
                
                // Estimate how many times a nonterminal was is in the current sentence
                for (Symbol nt : grammar.get_nonterminals()) {
                    symbol_prob[nt] = estimate_symbol_expectation(nt, len, iocalc);
                }
                
                // Estimate how many times a rule is used.
                for (ProbabilisticContextFreeGrammar::iterator rule = grammar.begin(); rule != grammar.end(); ++rule) {
                    assert(symbol_prob.find(rule->get_lhs()) != symbol_prob.end()); 
                    
                    if (rule->arity() == 2) { // Normal rules -> (11.26), p. 400                        
                        // Divide the expectation for this rule by the expectation for the lhs symbol of this rule.
                        rule_prob[*rule] = estimate_rule_expectation((*rule), len, iocalc) / symbol_prob[rule->get_lhs()];
                        
                    } else { // Preterminal rules -> (11.27), p. 400
                        
                        rule_prob[*rule] = estimate_terminal_rule_expectation((*rule), len, cit->first, iocalc) / symbol_prob[rule->get_lhs()];
                    }
                    
                }
                
                // Maximize the probability of the rules in the grammar
//                for (ProbabilisticContextFreeGrammar::iterator rule = grammar.begin(); rule != grammar.end(); ++rule) {
//                    rule->set_probability(rule_prob[*rule]);
//                }
            }
        }
    }

    /// This function is an implementation of fig. (11.24) on p. 399 in Manning&Schuetze.
    /// It calculates the estimate for how many times a NT is used in the derivation of the current sentence.
    /// Note that in contrast to the version in 11.24, we do not divide by the inside of the whole sentence,
    /// because this step is not needed in further steps but we are rather interested in the undivided result.
    Probability estimate_symbol_expectation(const Symbol& symbol, unsigned len, InsideOutsideCalculator& iocalc) {
        // Iterate over all possible ranges in the sentence
//        std::cerr << "Symbol: " << signature.resolve_id(symbol) << "\n";
        Probability score = 0;
        for (unsigned p = 0; p < len; ++p) {
            for (unsigned q = p; q < len; ++q) {
                Probability current_outside = iocalc.calculate_outside(symbol, p, q);
                Probability current_inside = iocalc.calculate_inside(symbol, p, q);
                
                Probability current_result = (current_outside * current_inside);
//                std::cerr << "p=" << p << "q=" <<q << "\n";
//                std::cerr << "Outside: " << current_outside << " | Inside: " << current_inside << " | " << " Pi: " << pi << " | Result: " << current_result << "\n";
                score += current_result;
            }
        }
        LOG(INFO) << "EMTrainer: Estimation for the symbol '" << signature.resolve_id(symbol) << "' is "<<score;
        return score;
    }
    
    /// Like estimate_symbol_expectationm, but for rules. See fig. (11.25) on p. 400 in Manning&Schuetze.
    /// Again, we do not divide the result by the inside probability of the whole sentence.
    Probability estimate_rule_expectation(const PCFGRule& rule, unsigned len, InsideOutsideCalculator& iocalc) {
        assert(rule.arity() == 2);
        Probability score = 0;
        
        std::cout << "len: " << len << "\n";
        for (unsigned p = 0; p < len-1; ++p) {
            for (unsigned q = p+1; q < len; ++q) {
                for (unsigned d = p; d < q; ++d) {
                    Probability outside_lhs = iocalc.calculate_outside(rule.get_lhs(), p, q);
                    Probability inside_rhs1 = iocalc.calculate_inside(rule[0], p, d);
                    Probability inside_rhs2 = iocalc.calculate_inside(rule[1], d+1, q);
                    
                    Probability current_score = rule.get_prob() * outside_lhs * inside_rhs1 * inside_rhs2;
                    
                    std::cerr << "p=" << p << " q="<<q << " d="<<d << "\n";
                    std::cerr << "outside_lhs=" << outside_lhs << " inside_rhs1=" << inside_rhs1 << " inside_rhs2=" << inside_rhs2 << "\n";
                    
                    score += current_score;
                }
            }
        }
        
        return score;
    }
    
    /// Like estimate_symbol_expectationm but for terminal rules. 
    /// See Manning&Schuetze: p.400, (11.27). This function implements the numerator of the fraction,
    /// as the denumerator has been calculated in advance.
    Probability estimate_terminal_rule_expectation(const PCFGRule& rule, unsigned len, const SymbolVector& sentence, InsideOutsideCalculator& iocalc) {
        assert(rule.arity() == 1);
        
        Probability score = 0;
        
        for (unsigned h = 0; h < len; ++h) {
            // P(w_h = w^k) - check, if the terminal in the sentence at position h and the the terminal on the rhs are the same.
            if (rule.get_rhs()[0] == sentence[h]) {
                
                Probability outside = iocalc.calculate_outside(rule.get_lhs(), h, h);
                Probability inside = iocalc.calculate_inside(rule.get_lhs(), h, h);
                
                std::cout << "OS: " << outside << "  IS: " << inside << "\n";
                
                score += outside * inside;
                
            } // else: add 0 to the score.
        }

        std::cout << "Returning : " << score << "\n";
        return score;
    }

    void read_in(std::istream& corpus) {
        std::string line;
        unsigned line_no = 1;
        LOG(INFO) << "EMTrainer: Reading in the training corpus...";
        while (corpus.good()) {
            std::getline(corpus, line);
            if (!line.empty()) {
                LOG(INFO) << "EMTRainer: Reading in line " << line_no << ": '" << line << "'.";
                // Tokenize line
                Tokenizer tokens(line, CharSeparator("\t "));
                SymbolVector tokens_id;;
                bool valid = true;
                
                for (ExternalSymbol word : tokens) {
                    Symbol word_as_id = signature.resolve_symbol(word);
                    if (word_as_id < 0) { // If a terminal symbol was not found, mark this sentence as invalid.
                        LOG(INFO) << "EMTRainer: Sentence in line " << line_no << " will be ignored, the token '" << word<< "' cannot be resolved.";
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
    
    std::string symbol_vector_to_string(const SymbolVector& vector) {
        std::stringstream sstream;
        
        for (Symbol sym : vector) {
            sstream << signature.resolve_id(sym) << " ";
        }
        
        return sstream.str();
    }
    
private:
    ProbabilisticContextFreeGrammar& grammar;
    InsideOutsideCalculator* iocalc;
    Signature<ExternalSymbol>& signature;
    unsigned no_of_sentences;
    SentencesVector sentences;
    
};

#endif	/* EMTRAINER_HPP */

