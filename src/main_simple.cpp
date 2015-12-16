//
//  main.cpp
//  PCFG-EM
//
//  Created by Johannes Gontrum on 26/08/14.
//
// #define _ELPP_DISABLE_LOGS
#define NDEBUG

#include <iostream>
#include <string>

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
    
    std::string training_file = "examples/test_training.txt";
    std::string grammar_file = "examples/grammar.pcfg";
    unsigned iterations = 3;

    std::ifstream grammar_stream;
    grammar_stream.open(grammar_file, std::ios::in);

    std::ifstream training_stream;
    training_stream.open(training_file, std::ios::in);

    // Read in grammar
    ProbabilisticContextFreeGrammar grammar(grammar_stream);
                        
    // Initialize the EMTrainer
    EMTrainer trainer(grammar, training_stream);
    trainer.train(iterations);

    std::cout << grammar;    
}

