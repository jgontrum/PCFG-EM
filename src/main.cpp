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
    
    


//    
//    Signature<std::string> s;
//    PCFGRule r2("SBAR --> IN S [0.199324]", s);
//    PCFGRule r1("VP --> VBG ADVP-MNR [0.000418936]", s);
//    PCFGRule r3("VP --> ADVP-MNR VP|<VP-CC> [0.000558581]",s );
//    
//    std::vector<PCFGRule> vec;
//    vec.push_back(r1);
//    vec.push_back(r2);
//    vec.push_back(r3);
//
//    std::sort(vec.begin(), vec.end());
//
//    for (std::vector<PCFGRule>::const_iterator cit = vec.begin(); cit != vec.end(); ++cit) {
//        std::cerr << "|| " << *cit << "\n";
//    }
//    
//    std::cout << r1 << "\n";
//    std::cout << r2 << "\n";
//    std::cout << r3 << "\n";
//
//    std::cout << "r1 < r1?" << (r1 < r1) << "\n";
//    std::cout << "r1 < r2?" << (r1 < r2) << "\n";
//    std::cout << "r1 < r3?" << (r1 < r3) << "\n";
//
//    std::cout << "r2 < r1?" << (r2 < r1) << "\n";
//    std::cout << "r2 < r2?" << (r2 < r2) << "\n";
//    std::cout << "r2 < r3?" << (r2 < r3) << "\n";
//    
//    std::cout << "r3 < r1?" << (r3 < r1) << "\n";
//    std::cout << "r3 < r2?" << (r3 < r2) << "\n";
//    std::cout << "r3 < r3?" << (r3 < r3) << "\n";


    
    typedef boost::char_separator<char> CharSeparator;
    typedef boost::tokenizer<CharSeparator> Tokenizer;
    typedef std::vector<std::string> StringVector;
    
    
    // Define and parse the program options (-> http://www.radmangames.com/programming/how-to-use-boost-program_options)
    namespace po = boost::program_options;
    po::options_description desc("PCFG EMTraining Options");
    desc.add_options()
            ("help", "Print help messages")
            ("grammar,g", po::value<std::string>(), "Path to a PCFG.")
            ("train,t", po::value<std::string>(), "Path to the training set with sentences seperated by newlines.")
            ("save,s", po::value<std::string>(), "Path to save the altered grammar")
            ("out,o", "Output the grammar after the training.")
            ("iterations,i", po::value<std::string>(), "Amount of training circles to perform. (Default: 3)")
            ("vlevel,v=", po::value<std::string>(), "--v=2");
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
            std::string grammar_arg = vm["grammar"].as<std::string>();
//            std::cout << "'" << grammar_arg.substr(1, grammar_arg.size()) << "'\n" ;
            grammar_file.open(grammar_arg, std::ios::in);
            if (grammar_file) {

                std::ifstream training_file;
                std::string training_arg = vm["train"].as<std::string>();
                training_file.open(training_arg, std::ios::in);

                if (training_file) {
                    // Read in grammar
                    ProbabilisticContextFreeGrammar grammar(grammar_file);
                                        
                    // Initialize the EMTrainer
                    EMTrainer trainer(grammar, training_file);
                    
                    // Perform the actual training
                    if (vm.count("iterations")) {
                        std::string iterations_arg = vm["iterations"].as<std::string>();
                        unsigned it = boost::lexical_cast<unsigned>(iterations_arg);

                        trainer.train(it);
                    } else {
                        trainer.train(3); // default
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

