/* 
 * File:   EMTrainer.hpp
 * Author: Johannes Gontrum
 *
 * Created on September 17, 2014, 6:49 PM
 */

#ifndef EMTRAINER_HPP
#define	EMTRAINER_HPP

#include <iostream>

#include "ProbabilisticContextFreeGrammar.hpp"
#include "InsideOutsideCalculator.hpp"
#include "Signature.hpp"

#include <boost/tokenizer.hpp>

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
                
                iocalc.set_sentence(&(cit->first));
                unsigned len = (cit->first).size();
                
                // Calculate the inside probabiliy for the whole sentence first.
                // in M&S this varible is called "Pi" and defined as 
                // P(w_1m | G) = P(N^1 =>* w_1m | G) = Beta_1(1,m)
                Probability inside_sentence = iocalc.calculate_inside(grammar.get_start_symbol(), 0, len-1);
                LOG(INFO) << "EMTrainer: Inside Probability for the whole sentence is " << inside_sentence;
                
                // Estimate how many times a nonterminal was used in the current sentence
                for (Symbol nt : grammar.get_nonterminals()) {
                    Probability prob = estimate_symbol_expectation(nt, len, inside_sentence, iocalc);
                    std::cerr << prob << "!!!\n";
                }
            }
        }
    }

    /// This function is an implementation of fig. (11.24) on p. 399 in Manning&Schuetze.
    /// It calculates the estimate for how many times a NT is used in the derivation of the current sentence.
    Probability estimate_symbol_expectation(Symbol& symbol, unsigned len, Probability pi, InsideOutsideCalculator& iocalc) {
        // Iterate over all possible ranges in the sentence
        std::cerr << "Symbol: " << signature.resolve_id(symbol) << "\n";
        Probability score = 0;
        for (unsigned p = 0; p < len; ++p) {
            for (unsigned q = p; q < len; ++q) {
                Probability current_outside = iocalc.calculate_outside(symbol, p, q);
                Probability current_inside = iocalc.calculate_inside(symbol, p, q);
                
                Probability current_result = (current_outside * current_inside) / pi;
                std::cerr << "p=" << p << "q=" <<q << "\n";
                std::cerr << "Outside: " << current_outside << " | Inside: " << current_inside << " | " << " Pi: " << pi << " | Result: " << current_result << "\n";
                score += current_result;
            }
        }
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
//                StringVector tokens(tokens.begin(), tokens.end());
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
    
private:
    ProbabilisticContextFreeGrammar& grammar;
    InsideOutsideCalculator* iocalc;
    Signature<ExternalSymbol>& signature;
    unsigned no_of_sentences;
    SentencesVector sentences;
    
};

#endif	/* EMTRAINER_HPP */

