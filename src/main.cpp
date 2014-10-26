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
#include <boost/lexical_cast.hpp> // to cast a string to unsigned

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
    po::options_description desc("PCFG EMTraining Options");
    desc.add_options()
            ("help", "Print help messages")
            ("grammar,g", po::value<std::string>(), "Path to a PCFG.")
            ("corpus,c", po::value<std::string>(), "Path to the training set with sentences seperated by newlines.")
            ("save,s", po::value<std::string>(), "Path to save the altered grammar")
            ("out,o", "Output the grammar after the training.")
            ("iterations,i", po::value<unsigned>(), "Amount of training circles to perform. (Default: 3)")
            ("threshold,t", po::value<double>(), "The changes after the final iteration must be less equal to this value. Do not combine with  -i.")
            ("vlevel,v=", po::value<std::string>(), "Define the verbose level (0-10). E.g.: --v=2");
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("grammar")) {
        if (vm.count("corpus")) { 
            std::ifstream grammar_file;
            std::string grammar_arg = vm["grammar"].as<std::string>();

            grammar_file.open(grammar_arg, std::ios::in);
            if (grammar_file) {

                std::ifstream training_file;
                std::string training_arg = vm["corpus"].as<std::string>();
                training_file.open(training_arg, std::ios::in);

                if (training_file) {
                    // Read in grammar
                    ProbabilisticContextFreeGrammar grammar(grammar_file);
                                        
                    // Initialize the EMTrainer
                    EMTrainer trainer(grammar, training_file);
                    
                    // Perform the actual training
                    if (vm.count("iterations")) {
                        trainer.train(vm["iterations"].as<unsigned>());
                    } else if (vm.count("threshold")) {
                        trainer.train(vm["threshold"].as<double>());
                    } else {
                        trainer.train(unsigned(3)); // default
                    }

                    // If wanted, print the new grammar to cout
                    if (vm.count("out")) { 
                        std::cout << grammar;
                    }
                    // If wanted, save the new grammar in a file
                    if (vm.count("save")) {
                        std::string save_arg = vm["save"].as<std::string>();
                        
                        std::ofstream save_file(save_arg);
                        
                        if (save_file) {
                            save_file << grammar;
                            save_file.close();
                        } else {
                            std::cerr << "Could not write to file: '" << save_arg << "'";
                            return 1;
                        }
                    }
                } else {
                    std::cerr << "Could not read training data: '" << vm["train"].as<std::string>() << "'";
                    return 1;
                }
            } else {
                std::cerr << "Could not read PCFG: '" << vm["grammar"].as<std::string>() << "'";
                return 1;
            }
        } else {
            std::cerr << "Please specify a training file.\n";
            return 1;
        }
    } else {
        std::cerr << "Please specify a grammar file.\n\n" << desc << "\n";;
        return 1;
    }
    
}

