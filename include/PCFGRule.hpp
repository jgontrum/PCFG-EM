//////////////////////////////////////////////////////////////////////////////
// PCFGRule.hpp
// Represents probabilistic rules for a context-free grammar
// Johannes Gontrum, 16.9.14
//////////////////////////////////////////////////////////////////////////////'

#ifndef __PCFGRule_HPP__
#define __PCFGRule_HPP__

#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <cstddef> // for size_t
#include <boost/tokenizer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp> // to cast a string to double
#include "easylogging++.h"

class PCFGRule {
public: // Typedefs
    typedef uint32_t ID;
    typedef std::vector<ID> IDVector;
    typedef std::string ExternalSymbol;
    typedef double Probability;
    typedef Signature<ExternalSymbol> ExtSignature;

public:
    /// The default constructor creates an invalid rule

    PCFGRule() : valid(false) {
        this->signature = 0; // setting a null pointer for the signature
    }

    /// To create a valid rule, specify a string of the rule
    /// e.g. "S -> NP VP [1.0]" and a reference to a signature
    /// that translates the strings to numeric values.

    PCFGRule(const std::string& s, ExtSignature& signature) {
        this->signature = &signature;
        valid = parse_rule(s);
        if (valid) {
            LOG(INFO) << "PCFGRule: Rule for '" << *this << "' successfully created.";
        } else {
            LOG(WARNING) << "PCFGRule: Rule for '" << *this << "' could not be created.";
        }
    }
    

    //////////////////////////////////////////////////////////////////////////
    // Getter & Setter
    //////////////////////////////////////////////////////////////////////////

    const ID& get_lhs() const {
        return lhs;
    }

    const IDVector& get_rhs() const {
        return rhs;
    }

    const Probability& get_prob() const {
        return prob;
    }

    void set_probability(Probability& new_prob) {
        prob = new_prob;
    }

    /// returns the lenghth of the rhs

    const unsigned arity() const {
        return rhs.size();
    }

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////

    bool operator==(const PCFGRule& r) const {
        return lhs == r.lhs && rhs == r.rhs && prob == r.prob;
    }

    bool operator<(const PCFGRule& r) const {
        if (prob < r.prob) return true;
        if (lhs < r.lhs) return true;
        if (lhs == r.lhs) return rhs < r.rhs;
        return false;
    }

    /// a read-only index-operator to access the symbols on the rhs

    const ID operator[](unsigned pos) const {
        return (pos < rhs.size()) ? rhs[pos] : -1;
    }

    /// output-operator, prints the symbols as strings.

    friend std::ostream& operator<<(std::ostream& o, const PCFGRule& r) {
        assert((r.signature)->containsID(r.get_lhs()));
        o << (r.signature)->resolve_id(r.get_lhs()) << " -->";
        for (unsigned i = 0; i < r.arity(); ++i) {
            assert((r.signature)->containsID(r[i]));
            o << " " << (r.signature)->resolve_id(r[i]);
        }
        o << " [" << r.get_prob() << "]";
        return o;
    }

    // conversion operator, to allow statements like 'if (RULE) {...}'

    operator bool() const {
        return valid;
    }


    std::size_t hash() const {
        // left side first...
        std::size_t h = hash_int(lhs); // using size_t to calm down the hash_combine function...
        // now the right side
        for (unsigned i = 0; i < rhs.size(); ++i) {
            boost::hash_combine(h, rhs[i]);
        }
        // and eventually the probability
        boost::hash_combine(h, prob);
        return h;
    }

private:
    // parses a string like "S -> NP VP [1.0]"
    // Returns false, if there was a syntactic error

    bool parse_rule(const std::string& s) {
        if (signature != 0) {
            typedef boost::char_separator<char> CharSeparator;
            typedef boost::tokenizer<CharSeparator> Tokenizer;
            typedef std::vector<std::string> StringVector;

            Tokenizer tokens(s, CharSeparator("\t "));
            StringVector vtokens(tokens.begin(), tokens.end());
            if (vtokens.size() >= 3) {
                if (vtokens[1] != "-->") {
                    std::cerr << "PCFGRule: missing arrow in rule '" << s << "'\n";
                    return false;
                } else if (vtokens[0] == "-->") {
                    std::cerr << "PCFGRule: missing left-hand side in rule '" << s << "'\n";
                    return false;
                }
                // now check if the last item is a probability (e.g. [0.9])
                Tokenizer prob_token(vtokens[vtokens.size() - 1], CharSeparator("[]"));
                StringVector prob_vector(prob_token.begin(), prob_token.end());
                if (prob_vector.size() != 1) {
                    std::cerr << "PCFGRule: missing probability in '" << s << "'\n";
                    return false;
                } else {
                    // transform the strings to ints using the signature
                    lhs = signature->add_symbol(vtokens[0]);
                    vtokens.pop_back(); // remove the latest token (the probability)
                    for (StringVector::const_iterator cit = vtokens.begin() + 2; cit != vtokens.end(); ++cit) {
                        rhs.push_back(signature->add_symbol(*cit));
                    }
                    prob = boost::lexical_cast<double>(prob_vector[0]);

                }
            } else {
                std::cerr << "PCFGRule: Too few components in rule '" << s << "'\n";
                return false;
            }

            return true;
        } else {
            std::cerr << "PCFGRule: No valid signature given for rule '" << s << "'\n";
            return false;
        }

    }

    // Hashfunction for an integer. Code from: http://burtleburtle.net/bob/hash/integer.html
    uint32_t hash_int(uint32_t a) const {
        a = (a ^ 61) ^ (a >> 16);
        a = a + (a << 3);
        a = a ^ (a >> 4);
        a = a * 0x27d4eb2d;
        a = a ^ (a >> 15);
        return a;
    }

private: // Variables
    ID lhs; ///< Left side of the rule
    IDVector rhs; ///< Right side of the rule
    Probability prob; ///< The probability of this rule
    bool valid; ///< Is this rule correctly initialized?
    ExtSignature* signature; ///< Translates the internal used values to the external ones

};

#endif
