//
//  main.cpp
//  PCFG-EM
//
//  Created by Johannes Gontrum on 26/08/14.
//  Copyright (c) 2014 Universität Potsdam. All rights reserved.
//

#include <iostream>
#include <string>

#include <iostream>
#include <fstream>

#include <boost/unordered_set.hpp>
#include "../include/Signature.hpp"
#include "../include/ProbabilisticContextFreeGrammar.hpp"
#include "../include/PCFGRule.hpp"
#include "../include/InsideOutsideCalculator.hpp"
#include "../include/InsideOutsideCache.hpp"
#include "../include/EMTrainer.hpp"

#include "../include/easylogging++.h"
_INITIALIZE_EASYLOGGINGPP

int main(int argc, const char * argv[])
{
    _START_EASYLOGGINGPP(argc, argv);
    
    typedef boost::char_separator<char> CharSeparator;
    typedef boost::tokenizer<CharSeparator> Tokenizer;
    typedef std::vector<std::string> StringVector;

    std::ifstream grammar_file;

    grammar_file.open("examples/grammar_no_prob.pcfg", std::ifstream::in);

    if (grammar_file) {
        ProbabilisticContextFreeGrammar grammar(grammar_file);
        
        std::ifstream training_file;
        training_file.open("examples/test_training.txt", std::ifstream::in);
        
        if (training_file) {
            
            EMTrainer trainer(grammar, training_file);
//            
//            std::string sentence = "Maria mag Hans";
//            Tokenizer tokens(sentence, CharSeparator("\t "));
//            StringVector vtokens(tokens.begin(), tokens.end());
//            InsideOutsideCache io_cache(grammar);
//
//
//            std::string sentence2 = "mag";
//            Tokenizer tokens2(sentence, CharSeparator("\t "));
//            StringVector vtokens2(tokens.begin(), tokens.end());
//
//
//            InsideOutsideCalculator em(io_cache, vtokens2);
//
//            std::cout << em.calculate_outside(grammar.get_signature().resolve_symbol("V"), 1, 1);
//            //        std::cout << em.calculate_inside(grammar.get_start_symbol(), 0, vtokens.size() - 1);
            
        } else {
            std::cerr << "error reading file training";
        }
    } else {
            std::cerr << "error reading file ";
    }
    // Signature<std::string> sig;
    // PCFGRule r1("S --> NP VP [0.3]", sig);
    // std::cerr << r1 << "\n";
    // std::cerr << sig;

}

