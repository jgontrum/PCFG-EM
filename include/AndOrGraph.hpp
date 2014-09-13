////////////////////////////////////////////////////////////////////////////////
// AndOrGraph.hpp
// Und-Oder-Graph in C++
// TH, 30.6.2014
////////////////////////////////////////////////////////////////////////////////

#ifndef __AND_OR_GRAPH_HPP__
#define __AND_OR_GRAPH_HPP__

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include <iostream>
#include <vector>
#include <set>

template<typename NODE>
class AndOrGraph
{
public:
  typedef NODE                          NodeType;
  typedef std::vector<NodeType>         AndNodes;
  typedef std::vector<AndNodes>         OrNodes;
  typedef std::pair<NodeType,OrNodes>   AndArc;

public:
  /// Fügt eine Kante in den Und-Oder-Graphen ein
  void add_arc(const NodeType& mother, const AndNodes& dtrs)
  {
    and_or_graph[mother].push_back(dtrs);
  }

  /// Fügt eine Kante in den Und-Oder-Graphen ein
  void add_node(const NodeType& n)
  {
    and_or_graph.insert(std::make_pair(n,OrNodes()));
  }

  const OrNodes& or_nodes(const NodeType& n) const
  {
    static const OrNodes no_or_nodes;
    typename Graph::const_iterator fn = and_or_graph.find(n);
    return (fn != and_or_graph.end()) ? fn->second : no_or_nodes;
  }

  /// Führt einen Postorder-Durchlauf auf dem Und-Oder-Graphen durch, 
  /// berechnet node_func auf jedem Knoten und speichert die Ergebnisse in
  /// result_map
  template<typename NODEFUNC>
  void postorder_visit(const NodeType& node, NODEFUNC& node_func,
                       typename NODEFUNC::NodeResultMap& result_map) const
  {
    // Prüfen, ob der Knoten im Graph vorhanden ist
    typename Graph::const_iterator fn = and_or_graph.find(node);
    if (fn != and_or_graph.end()) {
      node_func.register_node(node);
      const OrNodes& or_nodes = fn->second;
      // Ja, oder-Knoten durchlaufen
      for (OrNodes::const_iterator o = or_nodes.begin(); o != or_nodes.end(); ++o) {
        const AndNodes& and_nodes = *o;
        for (AndNodes::const_iterator a = and_nodes.begin(); a != and_nodes.end(); ++a) {
          if (node_func.explore_node(*a)) {
            postorder_visit(*a,node_func,result_map);
          }
        }
      } // for o
      // Alle Nachfolger besucht => Funktion auf den Knoten anwenden
      result_map.insert(std::make_pair(node,node_func(node,or_nodes)));
    } // if
  }
  
  /// Erzeugt eine dot-Ausgabe für den Graphen  
  void draw(std::ostream& dot_out, bool top_down=true) const
  {       
    std::map<NodeType,unsigned> join_node_counters;

    dot_out << "digraph FSM {\n";
    dot_out << "graph [font = \"Times\", rankdir=";
    dot_out << (top_down ? "TD" : "LR"); 
    dot_out << ", fontsize=14, center=1, orientation=Portrait];\n";
    dot_out << "node  [shape = rect, style=filled, color=blue, fontcolor=white]\n";
    dot_out << "edge  []\n\n";
    
    for (Graph::const_iterator n = and_or_graph.begin(); n != and_or_graph.end(); ++n) {
      dot_out << n->first << std::endl;
      const OrNodes& or_nodes = n->second;
      for (OrNodes::const_iterator o = or_nodes.begin(); o != or_nodes.end(); ++o) {        
        dot_out << "\t" << "and_" << (o-or_nodes.begin()) << "_" << n->first 
                << " [shape=Mrecord, width=.2, style=filled, color=gray, label=\"";
        
        // Draw or-node
        for (unsigned k = 0; k < o->size(); ++k) {
          dot_out << "<p" << k << "> " << "&#183;";
          if (k < o->size()-1) dot_out << " | "; 
        }
        dot_out << "\"]\n";
        
        // Draw or-edges
        dot_out << n->first << " -> " << "and_" << (o-or_nodes.begin()) << "_" << n->first 
                << " [style=dashed]" << std::endl;
        
        // Draw and-edges
        if (o->size() == 0) {
          // May be some special output?
        }
        else if (o->size() == 1) {
          dot_out << "and_" << (o-or_nodes.begin()) << "_" << n->first 
                  << ":" << "p" << 0 << " -> " << (*o)[0] << std::endl;
        }
        else {
          for (unsigned k = 0; k < o->size(); ++k) {
            dot_out << "and_" << (o-or_nodes.begin()) << "_" << n->first 
                  << ":" << "p" << k << " -> "
                  << (*o)[k] << " [label=\"" << (k+1) << "\"]" << std::endl;
          } // for k
        }  
      } // for o
    } // for n 
    dot_out << "}\n";
  }
  
private: // Types
  typedef boost::unordered_map<NodeType,OrNodes>  Graph;

private:
  Graph and_or_graph;
}; // AndOrGraph

#endif
