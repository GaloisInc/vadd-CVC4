/*********************                                                        */
/*! \file sygus_unif_rl.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haniel Barbosa, Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2018 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief sygus_unif_rl
 **/

#include "cvc4_private.h"

#ifndef __CVC4__THEORY__QUANTIFIERS__SYGUS_UNIF_RL_H
#define __CVC4__THEORY__QUANTIFIERS__SYGUS_UNIF_RL_H

#include <map>
#include "theory/quantifiers/sygus/sygus_unif.h"

#include "theory/quantifiers/lazy_trie.h"
#include "theory/quantifiers_engine.h"

namespace CVC4 {
namespace theory {
namespace quantifiers {

using BoolNodePair = std::pair<bool, Node>;
using BoolNodePairHashFunction =
    PairHashFunction<bool, Node, BoolHashFunction, NodeHashFunction>;
using BoolNodePairMap =
    std::unordered_map<BoolNodePair, Node, BoolNodePairHashFunction>;
using NodePairMap = std::unordered_map<Node, Node, NodeHashFunction>;
using NodePair = std::pair<Node, Node>;

class CegConjecture;

/** Sygus unification Refinement Lemmas utility
 *
 * This class implement synthesis-by-unification, where the specification is a
 * set of refinement lemmas. With respect to SygusUnif, it's main interface
 * function is addExample, which adds a refinement lemma to the specification.
 */
class SygusUnifRl : public SygusUnif
{
 public:
  SygusUnifRl(CegConjecture* p);
  ~SygusUnifRl();

  /** initialize */
  void initialize(QuantifiersEngine* qe,
                  const std::vector<Node>& funs,
                  std::vector<Node>& enums,
                  std::map<Node, std::vector<Node>>& strategy_lemmas) override;
  /** Notify enumeration */
  void notifyEnumeration(Node e, Node v, std::vector<Node>& lemmas) override;
  /** Construct solution */
  bool constructSolution(std::vector<Node>& sols) override;
  /** add refinement lemma
   *
   * This adds a lemma to the specification. It returns the purified form
   * of the lemma based on the method purifyLemma below. The method adds the
   * head of "evaluation points" to the map eval_hds, where an evaluation point
   * is a term of the form:
   *   ev( e1, c1...cn )
   * where ev is an evaluation function for the sygus deep embedding, e1 is of
   * sygus datatype type (the "head" of the evaluation point), and c1...cn are
   * constants. If ev( e1, c1...cn ) is the purified form of ev( e, t1...tn ),
   * then e1 is added to eval_hds[e]. We add evaluation points to eval_hds only
   * for those terms that are newly generated by this call (and have not been
   * returned by a previous call to this method).
   */
  Node addRefLemma(Node lemma, std::map<Node, std::vector<Node>>& eval_hds);
  /**
   * whether f is being synthesized with unification strategies. This can be
   * checked through wehether f has conditional or point enumerators (we use the
   * former)
    */
  bool usingUnif(Node f);

  /** retrieve the head of evaluation points for candidate c, if any */
  std::vector<Node> getEvalPointHeads(Node c);

 protected:
  /** reference to the parent conjecture */
  CegConjecture* d_parent;
  /* Functions-to-synthesize (a.k.a. candidates) with unification strategies */
  std::unordered_set<Node, NodeHashFunction> d_unif_candidates;
  /** construct sol */
  Node constructSol(Node f, Node e, NodeRole nrole, int ind) override;
  /** collects data from refinement lemmas to drive solution construction
   *
   * In particular it rebuilds d_app_to_pt whenever d_prev_rlemmas is different
   * from d_rlemmas, in which case we may have added or removed data points
   */
  void initializeConstructSol() override;
  /** initialize construction solution for function-to-synthesize f */
  void initializeConstructSolFor(Node f) override;
  /*
    --------------------------------------------------------------
        Purification
    --------------------------------------------------------------
  */
  /**
   * maps heads of applications of a unif function-to-synthesize to their tuple
   * of arguments (which constitute a "data point" aka an "evaluation point")
   */
  std::map<Node, std::vector<Node>> d_hd_to_pt;
  /** maps unif candidates to heads of their evaluation points */
  std::map<Node, std::vector<Node>> d_cand_to_eval_hds;
  /**
   * maps applications of unif functions-to-synthesize to the result of their
   * purification */
  std::map<Node, Node> d_app_to_purified;
  /** maps unif functions-to-synthesize to counters of heads of evaluation
   * points */
  std::map<Node, unsigned> d_cand_to_hd_count;
  /**
   * This is called on the refinement lemma and will rewrite applications of
   * functions-to-synthesize to their respective purified form, i.e. such that
   * all unification functions are applied over concrete values. Moreover
   * unification functions are also rewritten such that every different tuple of
   * arguments has a fresh function symbol applied to it.
   *
   * Non-unification functions are also equated to their model values when they
   * occur as arguments of unification functions.
   *
   * A vector of guards with the (negated) equalities between the original
   * arguments and their model values is populated accordingly.
   *
   * When the traversal encounters an application of a unification
   * function-to-synthesize it will proceed to ensure that the arguments of that
   * function application are constants (ensureConst becomes "true"). If an
   * applicatin of a non-unif function-to-synthesize is reached, the requirement
   * is lifted (ensureConst becomes "false"). This avoides introducing spurious
   * equalities in model_guards.
   *
   * For example if "f" is being synthesized with a unification strategy and "g"
   * is not then the application
   *   f(g(f(g(0))))=1
   * would be purified into
   *   g(0) = c1 ^ g(f1(c1)) = c3 => f2(c3)
   *
   * Similarly
   *   f(+(0,f(g(0))))
   * would be purified into
   *   g(0) = c1 ^ f1(c1) = c2 => f2(+(0,c2))
   *
   * This function also populates the maps between candidates, heads and
   * evaluation points
   */
  Node purifyLemma(Node n,
                   bool ensureConst,
                   std::vector<Node>& model_guards,
                   BoolNodePairMap& cache);
  /*
    --------------------------------------------------------------
        Strategy information
    --------------------------------------------------------------
  */
  /**
   * This class stores the necessary information for building a decision tree
   * for a particular node in the strategy tree of a candidate variable f.
   */
  class DecisionTreeInfo
  {
   public:
    DecisionTreeInfo() {}
    ~DecisionTreeInfo() {}
    /** initializes this class */
    void initialize(Node cond_enum,
                    SygusUnifRl* unif,
                    SygusUnifStrategy* strategy,
                    unsigned strategy_index);
    /** adds the respective evaluation point of the head f  */
    void addPoint(Node f);
    /** adds a condition value to the pool of condition values */
    void addCondValue(Node condv);
    /** returns index of strategy information of strategy node for this DT */
    unsigned getStrategyIndex() const;
    /** builds solution stored in DT, if any, using the given constructor
     *
     * The DT contains a solution when no class contains two heads of evaluation
     * points with different model values, i.e. when all points that must be
     * separated indeed are separated.
     *
     * This function tests separation of the points in the above sense and may
     * create separation lemmas to enforce guide the synthesis of conditons that
     * will separate points not currently separated.
     */
    Node buildSol(Node cons);
    /** whether all points that must be separated are separated **/
    bool isSeparated();
    /** reference to parent unif util */
    SygusUnifRl* d_unif;
    /** enumerator template (if no templates, nodes in pair are Node::null()) */
    NodePair d_template;
    /** enumerated condition values */
    std::vector<Node> d_conds;

   private:
    /**
     * reference to inferred strategy for the function-to-synthesize this DT is
     * associated with
     */
    SygusUnifStrategy* d_strategy;
    /** index of strategy information of strategy node this DT is based on
     *
     * this is the index of the strategy (d_strats[index]) in the strategy node
     * to which this decision tree corresponds, which can be accessed through
     * the above strategy reference
     */
    unsigned d_strategy_index;
    /**
     * The enumerator in the strategy tree that stores conditions of the
     * decision tree.
     */
    Node d_cond_enum;
    /** chache of model values of heads of evaluation points */
    NodePairMap d_hd_values;
    /** Classifies evaluation points according to enumerated condition values
     *
     * Maintains the invariant that points evaluated in the same way in the
     * current condition values are kept in the same "separation class."
     */
    class PointSeparator : public LazyTrieEvaluator
    {
     public:
      /** initializes this class */
      void initialize(DecisionTreeInfo* dt);
      /**
       * evaluates the respective evaluation point of the head n on the index-th
       * condition
       */
      Node evaluate(Node n, unsigned index) override;

      /** the lazy trie for building the separation classes */
      LazyTrieMulti d_trie;

     private:
      /** reference to parent unif util */
      DecisionTreeInfo* d_dt;
    };
    /**
     * Utility for determining how evaluation points are separated by currently
     * enumerated condiotion values
     */
    PointSeparator d_pt_sep;
  };
  /** maps strategy points in the strategy tree to the above data */
  std::map<Node, DecisionTreeInfo> d_stratpt_to_dt;
  /** maps conditional enumerators to strategy points in which they occur */
  std::map<Node, std::vector<Node>> d_cenum_to_stratpt;
  /** maps unif candidates to their conditional enumerators */
  std::map<Node, std::vector<Node>> d_cand_cenums;
  /** all conditional enumerators */
  std::vector<Node> d_cond_enums;
  /** register strategy
   *
   * Initialize the above data for the relevant enumerators in the strategy tree
   * of candidate variable f.
   *
   * Lemmas to remove redundant operators from enumerators of specific strategy
   * points, if any, are retrived from strategy_lemmas.
   */
  void registerStrategy(Node f,
                        std::map<Node, std::vector<Node>>& strategy_lemmas);
  /** register strategy node
   *
   * Called while traversing the strategy tree of f. The arguments e and nrole
   * indicate the current node in the tree we are traversing, and visited
   * indicates the nodes we have already visited.
   *
   * Lemmas to remove redundant operators from enumerators of specific strategy
   * points, if any, are retrived from strategy_lemmas.
   */
  void registerStrategyNode(Node f,
                            Node e,
                            NodeRole nrole,
                            std::map<Node, std::map<NodeRole, bool>>& visited,
                            std::map<Node, std::vector<Node>>& strategy_lemmas);
  /** register conditional enumerator
   *
   * Registers that cond is a conditional enumerator for building a (recursive)
   * decision tree at strategy node e within the strategy tree of f.
   *
   * Lemmas to remove redundant operators from enumerators of specific strategy
   * points, if any, are retrived from strategy_lemmas.
   */
  void registerConditionalEnumerator(
      Node f,
      Node e,
      Node cond,
      unsigned strategy_index,
      std::map<Node, std::vector<Node>>& strategy_lemmas);
};

} /* CVC4::theory::quantifiers namespace */
} /* CVC4::theory namespace */
} /* CVC4 namespace */

#endif /* __CVC4__THEORY__QUANTIFIERS__SYGUS_UNIF_RL_H */
