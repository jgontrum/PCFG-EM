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

#include "../include/easylogging++.h"
_INITIALIZE_EASYLOGGINGPP

int main(int argc, const char * argv[])
{

    typedef boost::char_separator<char> CharSeparator;
    typedef boost::tokenizer<CharSeparator> Tokenizer;
    typedef std::vector<std::string> StringVector;

    std::ifstream file;
    file.open("examples/grammar.pcfg", std::ifstream::in);
    if (file) {
        ProbabilisticContextFreeGrammar grammar(file);
        std::cout << grammar;
        std::cout << "\nValid? " << grammar.is_valid_pcfg();

        std::string sentence = "Maria mag Hans";
        Tokenizer tokens(sentence, CharSeparator("\t "));
        StringVector vtokens(tokens.begin(), tokens.end());
        
        InsideOutsideCalculator em(grammar, vtokens);

    } else {
            std::cerr << "error reading file ";
    }
    // Signature<std::string> sig;
    // PCFGRule r1("S --> NP VP [0.3]", sig);
    // std::cerr << r1 << "\n";
    // std::cerr << sig;

}

