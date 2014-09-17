/* 
 * File:   InsideOutsideTools.hpp
 * Author: Johannes Gontrum
 *
 * Created on September 17, 2014, 6:51 PM
 */

#ifndef INSIDEOUTSIDETOOLS_HPP
#define	INSIDEOUTSIDETOOLS_HPP

#include "ProbabilisticContextFreeGrammar.hpp"
#include "Signature.hpp"
#include "PCFGRule.hpp"
#include <vector>
#include <string>

class InsideOutsideTools {
public:
    typedef PCFGRule::Probability                               Probability; 
    typedef ProbabilisticContextFreeGrammar::ExternalSymbol     ExternalSymbol;
    typedef std::string                                         String;
    typedef std::vector<ExternalSymbol>                         ExtVector;   
    typedef ProbabilisticContextFreeGrammar::Symbol             Symbol;
    typedef std::vector<Symbol>                                 SymbolVector;
    
public:
    InsideOutsideCalculator(ProbabilisticContextFreeGrammar& grammar, ExtVector& sentence) {
        pcfg = &grammar;
        signature = &pcfg.get_signature();
        // create the input vector, that contains numeric values instead of strings 
        for (ExternalSymbol word : sentence) {
            input.push_back(signature.add_symbol(word));
        }
    }
    
    Probability calculate_inside(int begin, int end) {
        return 0;
    }
    
private:
    ProbabilisticContextFreeGrammar&                    pcfg;
    ProbabilisticContextFreeGrammar::ExtSignature&      signature;
    SymbolVector                                        input;
};

#endif	/* INSIDEOUTSIDETOOLS_HPP */

