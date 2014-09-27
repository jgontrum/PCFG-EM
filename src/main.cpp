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
#include <boost/program_options.hpp>

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
    
    
    // Define and parse the program options (-> http://www.radmangames.com/programming/how-to-use-boost-program_options)
    namespace po = boost::program_options;
    po::options_description desc;
    desc.add_options()
            ("help", "Print help messages")
            ("grammar,g", po::value<std::string>(), "Path to a PCFG.")
            ("train,t", po::value<std::string>(), "Path to the training set with sentences seperated by newlines.")
            ("save,s", po::value<std::string>(), "Path to save the altered grammar")
            ("out,o", "Output the grammar after the training.")
            ("iterations,i", po::value<unsigned>(), "Amount of training circles to perform. (Default: 3)")
            ;

    po::variables_map vm; 
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("grammar")) {
        if (vm.count("train")) { 
            std::ifstream grammar_file;
            grammar_file.open(vm["grammar"].as<std::string>(), std::ifstream::in);
            if (grammar_file) {

                std::ifstream training_file;
                training_file.open(vm["train"].as<std::string>(), std::ifstream::in);

                if (training_file) {
                    // Read in grammar
                    ProbabilisticContextFreeGrammar grammar(grammar_file);

                    // Initialize the EMTrainer
                    EMTrainer trainer(grammar, training_file);
                    
                    // Perform the actual training
                    if (vm.count("iterations")) {
                        trainer.train(vm["iterations"].as<unsigned>());
                    } else {
                        trainer.train(3); // default
                    }

                    // If wanted, print the new grammar to cout
                    if (vm.count("out")) { 
                        std::cout << grammar;
                    }
                    // If wanted, save the new grammar in a file
                    if (vm.count("save")) {
                        // ...
                    }
                } else {
                    std::cerr << "Could not read training data.";
                }
            } else {
                std::cerr << "Could not read PCFG.";
            }
        } else {
            std::cerr << "Please specify a training file.\n";
            return 1;
        }
    } else {
        std::cerr << "Please specify a grammar.\n";
    }
    
}

