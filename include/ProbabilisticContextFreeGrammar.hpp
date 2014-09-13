////////////////////////////////////////////////////////////////////////////////
// ContextFreeGrammar.hpp
// Einfache kontextfreie Grammatikklasse in C++
// TH, 16.6.2014
////////////////////////////////////////////////////////////////////////////////

#ifndef __CONTEXTFREEGRAMMAR_HPP__
#define __CONTEXTFREEGRAMMAR_HPP__

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include <boost/unordered_set.hpp>

#include "CFGRule.hpp"
#include "AndOrGraph.hpp"

/// ContextFreeGrammar implementiert eine ungewichtete kontextfreie Grammatik
/// Die Klasse kann allein operieren oder zusammen mit einem Lexikon. Im ersten Fall
/// ist das Lexikon durch zusätzliche Regeln POS --> WORD gegeben 
class ContextFreeGrammar
{
public: // Typdefinitionen
  typedef CFGRule::Symbol               Symbol;
  typedef boost::unordered_set<Symbol>  SymbolSet;

private: // Typdefinitionen
  typedef SymbolSet::const_iterator     SymbolSetIter;
  typedef std::vector<CFGRule>          RuleVector;

public: // Typdefinitionen
  /// Externer Iterator, sowohl für alle Regeln als auch Teilbereiche
  /// (ist hinter den Kulissen ein Vektor-Iterator)
  typedef RuleVector::const_iterator                const_iterator;
  /// Ein Teilbereich unserer sortierten Regeln wird durch ein Paar von
  /// Iteratoren abgesteckt, die ein halboffenes Intervall definieren
  typedef std::pair<const_iterator,const_iterator>  LHSRange;

private: // Typdefinitionen
  /// Typ der internen Regel-Index-Struktur
  typedef std::map<Symbol,LHSRange>                 RuleIndex;

public: // Funktionen  
  /// Konstruktor aus input stream (Datei, cin, etc.)
  ContextFreeGrammar(std::istream& grm_in, bool only_grammar=false)
  : only_grammar_rules(only_grammar)
  {
    read_in(grm_in);
  }  

  /// Gibt das festgelegte Startsymbol zurück
  const Symbol& get_start_symbol() const
  {
    return start_symbol;
  }

  /// Setzt das Startsymbol der Grammatik
  void set_start_symbol(const Symbol& start)
  {
    start_symbol = start;
    nonterminal_symbols.insert(start);
  }

  /// Gibt true zurück, wenn sym die linke Seite einer Regel bildet
  bool is_nonterminal(const Symbol& sym) const
  {
    return nonterminal_symbols.find(sym) != nonterminal_symbols.end();
  }

  /// Gibt true zurück, wenn sym die linke Seite einer Regel bildet
  bool is_preterminal(const Symbol& sym) const
  {
    if (only_grammar_rules)
      return nonterminal_symbols.find(sym) == nonterminal_symbols.end();
    else return false; // TODO
  }

  /// Gibt true zurück, wenn sym die linke Seite einer Regel bildet
  bool is_terminal(const Symbol& sym) const
  {
    return nonterminal_symbols.find(sym) == nonterminal_symbols.end();
  }

  /// Gibt für die linke Seite 'lhs' ein Paar von Regelvektoriteratoren zurück,
  /// die ein halboffenes Intervall begrenzen, das die Regeln enthält, die
  /// 'lhs' expandieren
  LHSRange rules_for(const Symbol& lhs) const
  {
    // Anm. LHSRange ist ein ziemlich einfaches Objekt, so dass man es auch
    // per Wert (also nicht per Referenz zurückgeben kann)
    RuleIndex::const_iterator f_lhs = rule_index.find(lhs);
    return (f_lhs != rule_index.end()) ? f_lhs->second : LHSRange(end(),end());
  }

  /// Gibt einen Iterator auf den Beginn der Regelmenge zurück
  const_iterator begin() const
  {
    return productions.begin();
  }
    
  /// Gibt einen Iterator auf das Element nach dem Ende der Regelmenge zurück
  const_iterator end() const
  {
    return productions.end();
  }

  /// Gibt die Anzahl der Regeln zurück
  unsigned no_of_rules() const
  {
    return productions.size();
  }

  /// Gibt die Anzahl der Nichtterminale zurück
  unsigned no_of_nonterminals() const
  {
    return nonterminal_symbols.size();
  }

  /// Nicht sehr effizient, da das jedesmal aufs Neue berechnet wird.
  SymbolSet alphabet() const
  {
    SymbolSet sigma;
    for (SymbolSetIter s = vocabulary.begin(); s != vocabulary.end(); ++s) {
      if (!is_nonterminal(*s)) sigma.insert(*s);
    }
    return sigma;
  }

  /// Bestimmt, ob die Grammatik in (erweiterter) CNF ist
  bool is_in_cnf() const
  {
    // Alle Regeln durchgehen
    for (const_iterator r = begin(); r != end(); ++r) {
      const CFGRule& rule = *r;
      if (rule.arity() == 0) {
        // Nur das Startsymbol darf tilgen
        if (rule.get_lhs() != get_start_symbol())
          return false;
      }
      else if (rule.arity() == 1) {
        // Unäre Regel, dann muss das Symbol ein Terminal sein
        if (is_nonterminal(rule[0]))
          return false; // Kettenregel gefunden
      }
      else if (rule.arity() == 2) {
        // Binäre Regel => beide Töchter müssen Nichtterminale sein
        if (!is_nonterminal(rule[0]) || !is_nonterminal(rule[1]))
          return false;
      }
      else return false;
    } // for
    return true;
  }

  /// Bestimmt, ob die Grammatik in GNF ist
  bool is_in_gnf() const
  {
    // Alle Regeln durchgehen
    for (const_iterator r = begin(); r != end(); ++r) {
      const CFGRule& rule = *r;
      if (rule.arity() == 0)
        return false; // Keine Tilgungsregeln erlaubt
      else { // |rhs| > 0
        if (is_nonterminal(rule[0]))
          return false; // GNF-Regeln müssen mit einem Terminal beginnen
        // Restliche Symbole durchgehen, alle müssen Nichtterminale sein
        for (unsigned i = 1; i < rule.arity(); ++i) {
          if (!is_nonterminal(rule[i])) return false;
        }
      }
    } // for
    return true;
  }

  /// Gibt true zurück, wenn die Regel Kettenregeln A --> B enthält
  bool has_chain_rules() const
  {
    for (const_iterator r = begin(); r != end(); ++r) {
      if (r->arity() == 1 && is_nonterminal((*r)[0])) return true;
    }
    return false;
  }

  /// Gibt true zurück, wenn die Grammatik Tilgungsregeln A --> eps enthält
  bool has_epsilon_rules() const
  {
    for (const_iterator r = begin(); r != end(); ++r) {
      if (r->arity() == 0) return true;
    }
    return false;    
  }

  /// Gibt true zurück, wenn die Grammatik direkt oder indirekt linksrekursiv ist
  bool is_left_recursive() const
  {
    // TODO
    return false;
  }

  /// Konzeptuell externer Ausgabeoperator, steht hier, damit er zum Freund
  /// von ContextFreeGrammar gemacht werden kann.
  friend std::ostream& operator<<(std::ostream& o, const ContextFreeGrammar& g)
  {
    // Man beachte zweierlei:
    // 1. Das "g.": wir sind konzeptuell nicht in der Klasse
    // 2. Das "print()": wegen friend haben wir Zugriff auf die private Funktion
    g.print(o);
    return o;
  }

private:  // Funktionen
  /// Liest die Grammatik ein
  bool read_in(std::istream& grm_in)
  {
    std::string line;
    unsigned line_no = 1;
    bool first_rule = true;
    
    // Evtl. noch productions.reserve() ausführen, wenn man eine ungefähre
    // Vorstellung über die Anzahl der Regeln in der Datei hat
    // Hinweis: man kennt die Größe der Textdatei, kann damit die Anzahl der Rgeln abschätzen
    
    // Regeln aus Datei einlesen
    while (grm_in.good()) {
      std::getline(grm_in,line);
      if (!line.empty() && line[0] != '#') {
        CFGRule r(line);
        if (r) {
          add_rule(r);
          if (first_rule) {
            // Die linke Seite der ersten Grammatikregel wird das Startsymbol
            set_start_symbol(r.get_lhs());
            first_rule = false;
          }
        }
        else {
          std::cerr << "CFG: Warning (line " << line_no << "): rule ignored\n";
        }
      }
      ++line_no;
    } // while
    
    // Jetzt mittels < von CFGRule sortieren ...
    std::sort(productions.begin(),productions.end());
    // ... und Regelindex aufbauen 
    build_rule_index();
    return true;
  }

  /// Füge eine Regel r zur Regelmenge hinzu
  void add_rule(const CFGRule& r)
  {
    // An den Regelvektor anfügen
    productions.push_back(r);
    // Füge die linke Regelseite zu den Nichtterminalen und zum Vokabular hinzu
    nonterminal_symbols.insert(r.get_lhs());
    vocabulary.insert(r.get_lhs());
    // Füge die Symbole der gesamten rechten Regelseite ins Vokabular ein
    vocabulary.insert(r.get_rhs().begin(),r.get_rhs().end());
  }

  /// Baut den Regelindex auf: für jedes Nichtterminal N enthält er
  /// ein Paar von Iteratoren, welches den Bereich für N im Regelvektor 
  /// als halboffenes Intervall absteckt.
  void build_rule_index()
  {
    if (!productions.empty()) {
      // Erstes LHS-Symbol im Regelvektor bestimmen
      Symbol current_lhs = begin()->get_lhs();
      // Anfang des Bereichs für dieses Symbol
      const_iterator left = begin();
      for (const_iterator r = begin(); r != end(); ++r) {
        if (r->get_lhs() != current_lhs) {
          // Nichtterminalbereich ist hier zu Ende
          rule_index[current_lhs] = LHSRange(left,r);
          // current_lhs ist nun das neue LHS-Symbol
          current_lhs = r->get_lhs();
          // r wird nun zum Anfang des neuen Bereichs
          left = r;
        }
      } // for
      // Der letzte Bereich muss noch eingetragen werden
      rule_index[current_lhs] = LHSRange(left,end());
    }
  }

  /// Gib die Grammatik auf einem Stream o aus
  void print(std::ostream& o) const
  {
    o << "(\n{";
    // Terminale ausgeben (das sind alle Vokabularsymbole abzgl. der Nichtterminale)
    for (SymbolSetIter s = vocabulary.begin(); s != vocabulary.end(); ++s) {
      // Test ob *s ein Nichtterminal ist
      if (!is_nonterminal(*s)) {
        // Ausgabe nur, wenn es keines ist
        o << *s << " ";
      }
    }
    o << "},\n";
    // Nichtterminale ausgeben
    print_symbol_set(o,nonterminal_symbols);
    o << ",\n";
    // Startsymbol
    o << start_symbol << ",\n{";
    // Regeln
    
    for (const_iterator r = begin(); r != end(); ++r) {
      o << *r << "\n";
    }
    o << "})";
  }

  void print_symbol_set(std::ostream& o, const SymbolSet& syms) const
  {
    o << "{";
    for (SymbolSetIter s = syms.begin(); s != syms.end();) {
      o << *s;
      if (++s != syms.end()) o << ",";
      else break; // Kein , nach letztem Element
    }
    o << "}";
  }

  void compute_nullable_nonterminals()
  {
    typedef AndOrGraph<Symbol>            CFGAndOrGraph;
    typedef CFGAndOrGraph::AndNodes       AndNodes;
    typedef CFGAndOrGraph::OrNodes        OrNodes;
    
    CFGAndOrGraph grm_and_or_graph;
    for (const_iterator r = begin(); r != end(); ++r) {
      grm_and_or_graph.add_arc(r->get_lhs(),AndNodes(r->get_rhs().begin(),r->get_rhs().end()));
    }

  }

private: // Instanzvariablen
  Symbol      start_symbol;             ///< Startsymbol
  SymbolSet   nonterminal_symbols;      ///< Nichtterminale
  SymbolSet   vocabulary;               ///< Terminale U Nichtterminale
  RuleVector  productions;              ///< Produktionsregeln
  RuleIndex   rule_index;               ///< Indexstruktur zum Auffinden von Regeln
  SymbolSet   nullable_nonterminals;
  bool        only_grammar_rules;       ///< If true then the grammar contains no lexicon rules
}; // ContextFreeGrammar

#endif
