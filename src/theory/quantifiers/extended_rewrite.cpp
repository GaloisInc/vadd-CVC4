/*********************                                                        */
/*! \file extended_rewrite.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Implementation of extended rewriting techniques
 **/

#include "theory/quantifiers/extended_rewrite.h"

#include "theory/arith/arith_msum.h"
#include "theory/datatypes/datatypes_rewriter.h"
#include "theory/quantifiers/term_util.h"
#include "theory/rewriter.h"
#include "theory/bv/theory_bv_utils.h"

using namespace CVC4::kind;
using namespace std;

namespace CVC4 {
namespace theory {
namespace quantifiers {

ExtendedRewriter::ExtendedRewriter()
{
  d_true = NodeManager::currentNM()->mkConst(true);
  d_false = NodeManager::currentNM()->mkConst(false);
}

Node ExtendedRewriter::extendedRewritePullIte(Node n)
{
  // generalize this?
  Assert(n.getNumChildren() == 2);
  Assert(n.getType().isBoolean());
  Assert(n.getMetaKind() != kind::metakind::PARAMETERIZED);
  std::vector<Node> children;
  for (unsigned i = 0; i < n.getNumChildren(); i++)
  {
    children.push_back(n[i]);
  }
  for (unsigned i = 0; i < 2; i++)
  {
    if (n[i].getKind() == kind::ITE)
    {
      for (unsigned j = 0; j < 2; j++)
      {
        children[i] = n[i][j + 1];
        Node eqr = extendedRewrite(
            NodeManager::currentNM()->mkNode(n.getKind(), children));
        children[i] = n[i];
        if (eqr.isConst())
        {
          std::vector<Node> new_children;
          Kind new_k;
          if (eqr == d_true)
          {
            new_k = kind::OR;
            new_children.push_back(j == 0 ? n[i][0] : n[i][0].negate());
          }
          else
          {
            Assert(eqr == d_false);
            new_k = kind::AND;
            new_children.push_back(j == 0 ? n[i][0].negate() : n[i][0]);
          }
          children[i] = n[i][2 - j];
          Node rem_eq = NodeManager::currentNM()->mkNode(n.getKind(), children);
          children[i] = n[i];
          new_children.push_back(rem_eq);
          Node nc = NodeManager::currentNM()->mkNode(new_k, new_children);
          Trace("q-ext-rewrite") << "sygus-extr : " << n << " rewrites to "
                                 << nc << " by simple ITE pulling."
                                 << std::endl;
          // recurse
          return extendedRewrite(nc);
        }
      }
    }
  }
  return Node::null();
}

Node ExtendedRewriter::extendedRewrite(Node n)
{
  n = Rewriter::rewrite( n );
  NodeManager * nm = NodeManager::currentNM();
  std::unordered_map<Node, Node, NodeHashFunction>::iterator it =
      d_ext_rewrite_cache.find(n);
  if (it == d_ext_rewrite_cache.end())
  {
    Trace("q-ext-rewrite-debug") << "Do extended pre-rewrite on : " << n << std::endl;
    
    Node ret = n;
    if (n.getNumChildren() > 0)
    {
      std::vector<Node> children;
      if (n.getMetaKind() == kind::metakind::PARAMETERIZED)
      {
        children.push_back(n.getOperator());
      }
      bool childChanged = false;
      for (unsigned i = 0; i < n.getNumChildren(); i++)
      {
        Node nc = extendedRewrite(n[i]);
        childChanged = nc != n[i] || childChanged;
        children.push_back(nc);
      }
      // Some commutative operators have rewriters that are agnostic to order,
      // thus, we sort here.
      if (TermUtil::isComm(n.getKind()))
      {
        childChanged = true;
        std::sort(children.begin(), children.end());
      }
      if (childChanged)
      {
        ret = nm->mkNode(n.getKind(), children);
      }
    }
    ret = Rewriter::rewrite(ret);
    Trace("q-ext-rewrite-debug") << "Do extended rewrite on : " << ret
                                 << " (from " << n << ")" << std::endl;

    Node new_ret;
    Kind k = ret.getKind();
    if (k == kind::EQUAL)
    {
      // simple ITE pulling
      new_ret = extendedRewritePullIte(ret);
    }
    else if (k == kind::ITE)
    {
      Assert(ret[1] != ret[2]);
      if (ret[0].getKind() == NOT)
      {
        ret = nm->mkNode( kind::ITE, ret[0][0], ret[2], ret[1]);
      }
      if (ret[0].getKind() == kind::EQUAL)
      {
        // simple invariant ITE
        for (unsigned i = 0; i < 2; i++)
        {
          if (ret[1] == ret[0][i] && ret[2] == ret[0][1 - i])
          {
            new_ret = ret[2];
            debugExtendedRewrite(ret,new_ret,"subs-ITE");
            break;
          }
        }
        // notice this is strictly more general than the above
        if (new_ret.isNull())
        {
          // simple substitution
          for (unsigned i = 0; i < 2; i++)
          {              
            TNode r1 = ret[0][i];
            TNode r2 = ret[0][1 - i];
            if (r1.isVar() && ((r2.isVar() && r1 < r2) || r2.isConst()))
            {
              Node retn = ret[1].substitute(r1, r2);
              if (retn != ret[1])
              {
                new_ret = nm->mkNode(ITE, ret[0], retn, ret[2]);
                debugExtendedRewrite(ret,new_ret,"subs-ITE");
              }
            }
          }
        }
      }
    }
    else if (k == DIVISION || k == INTS_DIVISION || k == INTS_MODULUS)
    {
      // rewrite as though total
      std::vector<Node> children;
      bool all_const = true;
      for (unsigned i = 0; i < ret.getNumChildren(); i++)
      {
        if (ret[i].isConst())
        {
          children.push_back(ret[i]);
        }
        else
        {
          all_const = false;
          break;
        }
      }
      if (all_const)
      {
        Kind new_k =
            (ret.getKind() == DIVISION
                 ? DIVISION_TOTAL
                 : (ret.getKind() == INTS_DIVISION ? INTS_DIVISION_TOTAL
                                                   : INTS_MODULUS_TOTAL));
        new_ret = nm->mkNode(new_k, children);
        debugExtendedRewrite(ret,new_ret,"total-interpretation");
      }
    }
    else if( k == BITVECTOR_AND || k == BITVECTOR_OR )
    {
      bool isAnd = ( k == BITVECTOR_AND );
      std::unordered_set< unsigned > drops;
      std::vector< Node > children;
      unsigned size=ret.getNumChildren();
      for( unsigned i=0; i<size; i++ )
      {
        Node cmpi = isAnd ? ret[i] : nm->mkNode( BITVECTOR_NOT, ret[i] );
        bool success = true;
        for( unsigned j=0; j<size; j++ )
        {
          if( i!=j && drops.find(j)==drops.end() )
          {
            bool cond = isAnd ? bitVectorSubsume( ret[i], ret[j] ) : bitVectorSubsume( ret[j], ret[i] );
            if( cond )
            {
              drops.insert(i);
              success = false;
            }
            Node cmpj = isAnd ? ret[j] : nm->mkNode( BITVECTOR_NOT, ret[j] );
            if( i<j && bitvectorDisjoint( cmpi, cmpj ) )
            {
              new_ret = mkConstBv( ret, !isAnd );
              debugExtendedRewrite( ret, new_ret, "AND/OR-disjoint" );
              break;
            }
          }
        }
        if( !new_ret.isNull() )
        {
          break;
        }
        if( success )
        {
          children.push_back(ret[i]);
        }
      }
      if( new_ret.isNull() )
      {
        Assert( !children.empty() );
        if( children.size()<size )
        {
          new_ret = children.size()==1 ? children[0] : nm->mkNode( k, children );
          debugExtendedRewrite( ret, new_ret, "AND/OR subsume" );
        }
      }
    }
    else if( k == BITVECTOR_ULT )
    {
      if( bitVectorArithComp( ret[0], ret[1] ) )
      {
        new_ret = nm->mkConst(false);
        debugExtendedRewrite( ret, new_ret, "ULT-arith" );
      }
    }
    else if( k == BITVECTOR_SLT )
    {
      if( ret[0]==ret[1] )
      {
        new_ret = nm->mkConst(false);
        debugExtendedRewrite( ret, new_ret, "SLT-id" );
      }
    }
    else if( k == BITVECTOR_LSHR || k == BITVECTOR_SHL )
    {
      new_ret = rewriteBvShift( ret );
    }
    else if( k == BITVECTOR_PLUS || k == BITVECTOR_MULT )
    {
      new_ret = rewriteBvArith( ret );
    }
    else if( k == BITVECTOR_NEG )
    {
      // miniscope
      if( ret[0].getKind()==BITVECTOR_SHL )
      {
        new_ret = nm->mkNode( BITVECTOR_SHL, nm->mkNode( BITVECTOR_NEG, ret[0][0] ), ret[0][1] );
        debugExtendedRewrite( ret, new_ret, "NEG-SHL-miniscope" );
      }
      else if( ret[0].getKind()==BITVECTOR_NOT )
      {
        // this should be handled by NOT-plus-miniscope below
        Assert( ret[0][0].getKind()!=BITVECTOR_PLUS || !hasConstBvChild( ret[0][0] ) );
      }
    }
    else if( k == BITVECTOR_NOT )
    {
      // ~( x+y ) ----> -(x+y)-1
      Kind ck = ret[0].getKind();
      if( ck==BITVECTOR_PLUS && hasConstBvChild( ret[0] ) )
      {
        Node max_bv = mkConstBv(ret[0],true);
        Node c = nm->mkNode( BITVECTOR_NEG, ret[0] );
        new_ret = nm->mkNode( BITVECTOR_PLUS, c, max_bv );
        debugExtendedRewrite( ret, new_ret, "NOT-plus-miniscope" );
      }
    }
    
    
    // more expensive rewrites
    if (new_ret.isNull())
    {
      Trace("q-ext-rewrite-debug2") << "Do expensive rewrites on " << ret
                                    << std::endl;
      bool polarity = ret.getKind() != NOT;
      Node ret_atom = ret.getKind() == NOT ? ret[0] : ret;
      if ((ret_atom.getKind() == EQUAL && ret_atom[0].getType().isReal())
          || ret_atom.getKind() == GEQ)
      {
        Trace("q-ext-rewrite-debug2") << "Compute monomial sum " << ret_atom
                                      << std::endl;
        // compute monomial sum
        std::map<Node, Node> msum;
        if (ArithMSum::getMonomialSumLit(ret_atom, msum))
        {
          for (std::map<Node, Node>::iterator itm = msum.begin();
               itm != msum.end();
               ++itm)
          {
            Node v = itm->first;
            Trace("q-ext-rewrite-debug2") << itm->first << " * " << itm->second
                                          << std::endl;
            if (v.getKind() == ITE)
            {
              Node veq;
              int res = ArithMSum::isolate(v, msum, veq, ret_atom.getKind());
              if (res != 0)
              {
                Trace("q-ext-rewrite-debug")
                    << "  have ITE relation, solved form : " << veq
                    << std::endl;
                // try pulling ITE
                new_ret = extendedRewritePullIte(veq);
                if (!new_ret.isNull())
                {
                  if (!polarity)
                  {
                    new_ret = new_ret.negate();
                  }
                  debugExtendedRewrite(ret,new_ret,"solve-ITE");
                  break;
                }
              }
              else
              {
                Trace("q-ext-rewrite-debug") << "  failed to isolate " << v
                                             << " in " << ret << std::endl;
              }
            }
          }
        }
        else
        {
          Trace("q-ext-rewrite-debug") << "  failed to get monomial sum of "
                                       << ret << std::endl;
        }
      }
      else if (ret_atom.getKind() == ITE)
      {
        // TODO : conditional rewriting
      }
      else if (ret.getKind() == kind::AND || ret.getKind() == kind::OR)
      {
        // TODO condition merging
      }
    }

    d_ext_rewrite_cache[n] = ret;
    if (!new_ret.isNull())
    {
      ret = extendedRewrite(new_ret);
    }
    d_ext_rewrite_cache[n] = ret;
    return ret;
  }
  else
  {
    return it->second;
  }
}  

Node ExtendedRewriter::rewriteBvArith( Node ret )
{
  Kind k = ret.getKind();
  Assert( k == BITVECTOR_PLUS || k == BITVECTOR_MULT );
  NodeManager * nm = NodeManager::currentNM();
  Node new_ret;
  unsigned size = bv::utils::getSize(ret);
  Node bv_one = bv::utils::mkOne(size);
  Node bv_neg_one = bv::utils::mkOnes(size);
  if( k == BITVECTOR_PLUS )
  {
    std::vector< Node > nconst;
    Node cc = getConstBvChild(ret, nconst);
    if( !cc.isNull() && ( cc==bv_one || cc==bv_neg_one ) )
    {
      Node rem = nconst.size()==1 ? nconst[0] : nm->mkNode( k, nconst );
      if( cc==bv_one )
      {
        // x+1 ---> -( ~x )
        new_ret = nm->mkNode( BITVECTOR_NEG, nm->mkNode( BITVECTOR_NOT, rem ) );
      }
      else if( cc==bv_neg_one )
      {
        // x-1 ---> ~( -x )
        new_ret = nm->mkNode( BITVECTOR_NOT, nm->mkNode( BITVECTOR_NEG, rem ) );
      }
    }
    if( !new_ret.isNull() )
    {
      debugExtendedRewrite( ret, new_ret, "arith-plus-elim" );
      return new_ret;
    }
  }
  
  std::vector< Node > rchildren;
  bool rchildrenChanged = false;
  for( const Node& rc : ret )
  {
    bool isNeg = rc.getKind()==BITVECTOR_NEG;
    Node rcc = isNeg ? rc[0] : rc;
    if( rcc.getKind()==BITVECTOR_NOT )
    {
      // ~x----> -(x+1)
      rcc = nm->mkNode( BITVECTOR_NEG, nm->mkNode( BITVECTOR_PLUS, rcc[0], bv_one ) );
      rchildrenChanged = true;
    }
    if( isNeg )
    {
      // negate it
      if( rcc.getKind()==BITVECTOR_NEG )
      {
        rcc = rcc[0];
      }
      else
      {
        rcc = nm->mkNode( BITVECTOR_NEG, rcc );
      }
    }
    rchildren.push_back( rcc );
  }
  if( rchildrenChanged )
  {
    new_ret = nm->mkNode( k, rchildren );
    debugExtendedRewrite( ret, new_ret, "arith-not-child" );
    return new_ret;
  }
  
  // general monomial normalization
  std::map< Node, Node > msum;
  std::map< Node, Node > shl;
  getBvMonomialSum( ret, msum, shl );
  
  Trace("q-ext-rewrite-bvarith") << "Monomial representation of " << ret << " : " << std::endl;
  for( const std::pair< Node, Node >& m : msum )
  {
    Trace("q-ext-rewrite-bvarith") << "  " << m.first << " * " << m.second;
    if( !shl[m.first].isNull() )
    {
      Trace("q-ext-rewrite-bvarith") << " >> " << shl[m.first];
    }
    Trace("q-ext-rewrite-bvarith") << std::endl;
  }
  
  new_ret = mkNodeFromBvMonomial( ret, msum, shl );
  Trace("q-ext-rewrite-bvarith") << "...got (pre-rewrite) : " << new_ret << std::endl;
  new_ret = Rewriter::rewrite( new_ret );
  if( new_ret!=ret )
  {
    debugExtendedRewrite( ret, new_ret, "arith-normalize" );
    return new_ret;
  }
  else
  {
    Trace("q-ext-rewrite-bvarith") << "Monomial " << ret << " did not normalize." << std::endl;
  }
  
  
  
  if( k == BITVECTOR_PLUS )
  {
    bool success = true;
    unsigned size=ret.getNumChildren();
    for( unsigned i=0; i<size; i++ )
    {
      for( unsigned j=(i+1); j<size; j++ )
      {
        if( !bitvectorDisjoint( ret[i], ret[j] ) )
        {
          success = false;
          break;
        }
      }
      if( !success )
      {
        break;
      }
    }
    if( success )
    {
      // if we prove disjointness, PLUS turns into OR
      // ( ( x ^ y ) ---> 0 ) => ( x+y ----> x V y )
      new_ret = nm->mkNode( BITVECTOR_OR, ret[0], ret[1] );
      debugExtendedRewrite( ret, new_ret, "PLUS-disjoint" );
      return new_ret;
    }
  }
  return Node::null();
}

Node ExtendedRewriter::rewriteBvShift( Node ret )
{
  Kind k = ret.getKind();
  Assert( k == BITVECTOR_LSHR || k == BITVECTOR_SHL );
  NodeManager * nm = NodeManager::currentNM();
  Node new_ret;
  
  std::vector< Node > cchildren;
  Node base = decomposeChain( k, ret, cchildren );
  
  if( k == BITVECTOR_LSHR )
  {
    std::vector< Node > bchildren;
    Kind bk = base.getKind();
    if( bk==BITVECTOR_OR || bk==BITVECTOR_XOR )
    {
      for( const Node& cr : base ) 
      {
        bchildren.push_back(cr);
      }
    }
    else
    {
      bchildren.push_back(base);
    }
    std::vector< Node > bctemp;
    bctemp.insert(bctemp.end(),bchildren.begin(),bchildren.end());
    bchildren.clear();
    bool childChanged = false;
    for( const Node& bc : bctemp )
    {
      // take into account NOT
      Node bca = bc.getKind()==BITVECTOR_NOT ? bc[0] : bc;
      bool shifted = false;
      for( const Node& cc : cchildren )
      {
        if( bitVectorArithComp( cc, bca ) )
        {
          shifted = true;
          break;
        }
      }
      // we are not able to shift away its 1-bits
      if( shifted )
      {
        // rewrite rule #20
        // if we shifted it away, then it might has well have been all 0's.
        Node const_bv = mkConstBv( bc, bc.getKind()==BITVECTOR_NOT );
        bchildren.push_back( const_bv );
        childChanged = true;
      }
      else
      {
        bchildren.push_back( bc );
      }
    }
    if( childChanged )
    {
      base = bchildren.size()==1 ? bchildren[0] : nm->mkNode( bk, bchildren );
    }
  }
  std::sort( cchildren.begin(), cchildren.end() );
  new_ret = mkChain( k, base, cchildren );
  if( new_ret!=ret )
  {
    debugExtendedRewrite( ret, new_ret, "shift-sort-arith" );
    return new_ret;
  }
  return Node::null();
}

bool ExtendedRewriter::bitVectorSubsume( Node a, Node b, bool strict )
{
  Assert( a.getType()==b.getType() );
  Trace("q-ext-rewrite-debug2") << "Subsume " << a << " " << b << "?" << std::endl;
  if( a==b )
  {
    return !strict;
  }
  if( a.isConst() && b.isConst() )
  {
    // TODO
  }
  if( a.getKind()==BITVECTOR_OR )
  {
    for( const Node& ac : a )
    {
      if( bitVectorSubsume( ac, b, strict ) )
      {
        return true;
      }
    }
  }
  else if( b.getKind()==BITVECTOR_AND )
  {
    for( const Node& bc : b )
    {
      if( bitVectorSubsume( a, bc, strict ) )
      {
        return true;
      }
    }
  }
  else if( b.getKind()==BITVECTOR_SHL )
  {
    if( b[0].getKind()==BITVECTOR_LSHR )
    {
      if( b[0][0]==a && b[0][1]==b[1] )
      {
        // or strict and geq zero
        return !strict;
      }
    }
  }
  else if( b.getKind()==BITVECTOR_LSHR )
  {
    if( b[0].getKind()==BITVECTOR_SHL )
    {
      if( b[0][0]==a && b[0][1]==b[1] )
      {
        // or strict and geq zero
        return !strict;
      }
    }
  }
  
  return false;
}

bool ExtendedRewriter::bitVectorArithComp( Node a, Node b, bool strict )
{
  Assert( a.getType()==b.getType() );
  if( bitVectorSubsume(a,b,strict) )
  {
    return true;
  }
  if( a.isConst() && b.isConst() )
  {
    // TODO
  }
  // shifting right always shrinks
  if( b.getKind() == BITVECTOR_LSHR )
  {
    if( bitVectorArithComp( a, b[0], strict ) )
    {
      return true;
    }
  }

  return false;
}

bool ExtendedRewriter::bitvectorDisjoint( Node a, Node b )
{
  Assert( a.getType()==b.getType() );
  if( a.isConst() && b.isConst() )
  {
    // TODO
  }
  for( unsigned r=0; r<2; r++ )
  {
    Node x = r==0 ? a : b;
    Node y = r==0 ? b : a;
    
    if( x.getKind()==BITVECTOR_NOT  )
    {
      if( x[0]==y )
      {
        return true;
      }
    }
    if( x.getKind()==BITVECTOR_SHL )
    {
      // bvshl( x, y ) is disjoint from z if y>z.
      if( bitVectorArithComp( x[1], y ) )
      {
        return true;
      }
    }
    else if( x.getKind()==BITVECTOR_AND )
    {
      for( const Node& xc : x ) 
      {
        if( bitvectorDisjoint( xc, y ) )
        {
          return true;
        }
      }
    }
  }
  
  return false;
}

Node ExtendedRewriter::decomposeChain( Kind k, Node n, std::vector< Node >& children )
{
  Node curr = n;
  while( curr.getKind()==k )
  {
    children.push_back( curr[1] );
    curr = curr[0];
  }
  return curr;
}

Node ExtendedRewriter::mkChain( Kind k, Node base, std::vector< Node >& children )
{
  NodeManager * nm = NodeManager::currentNM();
  Node curr = base;
  for( const Node& c : children )
  {
    curr = nm->mkNode( k, curr, c );
  }
  return curr;
}

Node ExtendedRewriter::mkConstBv( Node n, bool isNot )
{
  unsigned size = bv::utils::getSize(n);
  return isNot ? bv::utils::mkOnes(size) : bv::utils::mkZero(size);
}

bool ExtendedRewriter::isConstBv( Node n, bool isNot )
{
  if( n.isConst() )
  {
    Node const_n = mkConstBv( n, isNot );
    return n==const_n;
  }
  return false;
}

Node ExtendedRewriter::getConstBvChild( Node n, std::vector< Node >& nconst )
{
  Node ret;
  for( const Node& cn : n )
  {
    if( cn.isConst() )
    {
      Assert( ret.isNull() );
      ret = cn;
    }
    else
    {
      nconst.push_back( cn );
    }
  }
  return ret;
}

bool ExtendedRewriter::hasConstBvChild( Node n )
{
  for( const Node& cn : n )
  {
    if( cn.isConst() )
    {
      return true;
    }
  }
  return false;
}

void ExtendedRewriter::getBvMonomialSum( Node n, 
                                         std::map< Node, Node >& msum,
                                         std::map< Node, Node >& shl
                                       )
{
  NodeManager * nm = NodeManager::currentNM();
  std::vector< Node > sum;
  if( n.getKind() == BITVECTOR_PLUS )
  {
    for( const Node& cn : n )
    {
      sum.push_back( cn );
    }
  }
  else
  {
    sum.push_back( n );
  }
  std::map< Node, std::vector< Node > > msums;
  std::map< Node, std::vector< Node > > shls;
  unsigned size = bv::utils::getSize(n);
  Node bv_one = bv::utils::mkOne(size);
  for( const Node& cn : sum )
  {
    Node cmul;
    Node cshl;
    Node v = getBvMonomial( cn, cmul, cshl );
    Assert( v.isNull() || !v.isConst() );
    v = v.isNull() ? bv_one : v;
    cmul = cmul.isNull() ? bv_one : cmul;
    msums[v].push_back( cmul );
    if( !cshl.isNull() )
    {
      msums[v].push_back( cshl );
    }
  }
  for( const std::pair< Node, std::vector< Node > >& m : msums )
  {
    Node v = m.first;
    const std::vector< Node >& cs = m.second;
    Node mv = cs.size()==1 ? cs[0] : nm->mkNode( BITVECTOR_PLUS, cs );
    mv = Rewriter::rewrite( mv );
    Assert( mv.isConst() );
    msum[v] = mv;
    if( !shls[v].empty() )
    {
      Node sv = shls[v].size()==1 ? shls[v][0] : nm->mkNode( BITVECTOR_PLUS, shls[v] );
      sv = Rewriter::rewrite( sv );
      shl[v] = sv;
    }
  }
}

Node ExtendedRewriter::getBvMonomial( Node n, Node& cmul, Node& shl )
{
  Node ret;
  if( n.isConst() )
  {
    cmul = n;
    return ret;
  }
  NodeManager * nm = NodeManager::currentNM();
  Kind k = n.getKind();
  if( k==BITVECTOR_MULT )
  {
    std::vector< Node > nconst;
    std::vector< Node > cmuls;
    std::vector< Node > cshls;
    cmul = getConstBvChild( n, nconst );
    if( !cmul.isNull() )
    {
      cmuls.push_back( cmul );
    }
    // now extract product values from children
    std::vector< Node > cnconsts;
    for( unsigned i=0, size=nconst.size(); i<size; i++ )
    {
      Node cmulc;
      Node shlc;
      Node nc = getBvMonomial( nconst[i], cmulc, shlc );
      if( !nc.isNull() )
      {
        cnconsts.push_back( nc );
      }
      if( !cmulc.isNull() )
      {
        cmuls.push_back( cmulc );
      }
      if( !shlc.isNull() )
      {
        cshls.push_back( shlc );
      }
    }
    // sort monomial list
    std::sort( cnconsts.begin(), cnconsts.end() );
    if( !cnconsts.empty() )
    {
      ret = nm->mkNode( BITVECTOR_MULT, cnconsts );
    }
    if( !cmuls.empty() )
    {
      cmul = cmuls.size()==1 ? cmuls[0] : nm->mkNode( BITVECTOR_MULT, cmuls );
    }
    if( !cshls.empty() )
    {
      shl = cshls.size()==1 ? cshls[0] : nm->mkNode( BITVECTOR_MULT, cshls );
    }
  }
  else if( k==BITVECTOR_CONCAT && n.getNumChildren()==2 )
  {
    if( n[0].getKind()==BITVECTOR_EXTRACT && bv::utils::getExtractLow(n[0])==0 && isConstBv(n[1],false) )
    {
      ret = getBvMonomial( n[0][0], cmul, shl );
      unsigned size = bv::utils::getSize(n);
      // multiply by power of two
      unsigned sizez = bv::utils::getSize(n[1]);
      Integer powsizez = Integer(1).multiplyByPow2(sizez);
      Node coeff = bv::utils::mkConst(size,powsizez);
      if( cmul.isNull() )
      {
        cmul = coeff;
      }
      else
      {
        cmul = nm->mkNode( BITVECTOR_MULT, cmul, coeff );
      }
    }
    else
    {
      ret = n;
    }
  }
  else if( k==BITVECTOR_SHL )
  {
    ret = getBvMonomial( n[0], cmul, shl );
    // carry factor, equivalent to multiply by factor of 2^n[1]
    if( !shl.isNull() )
    {
      shl = n[1];
    }
    else
    {
      shl = nm->mkNode( BITVECTOR_PLUS, shl, n[1] );
    }
  }
  else if( k==BITVECTOR_NEG )
  {
    ret = getBvMonomial( n[0], cmul, shl );
    if( cmul.isNull() )
    {
      unsigned size = bv::utils::getSize(n);
      cmul = bv::utils::mkOne(size);
    }
    cmul = nm->mkNode( BITVECTOR_NEG, cmul );
  }
  else
  {
    ret = n;
  }
  return ret;
}  

Node ExtendedRewriter::mkNodeFromBvMonomial( Node n, std::map< Node, Node >& msum,
                          std::map< Node, Node >& shl )
{
  NodeManager * nm = NodeManager::currentNM();
  std::vector< Node > sum;
  for( const std::pair< Node, Node >& m : msum )
  {
    Node v = m.first;
    Node c = m.second;
    Node s = shl[v];
    Node mm = nm->mkNode( BITVECTOR_MULT, c, v );
    mm = Rewriter::rewrite( mm );
    if( !s.isNull() )
    {
      mm = nm->mkNode( BITVECTOR_SHL, mm, s );
    }
    sum.push_back( mm );
  }
  if( sum.empty() )
  {
    return mkConstBv( n, false );
  }
  else if( sum.size()==1 )
  {
    return sum[0];
  }
  return nm->mkNode( BITVECTOR_PLUS, sum );
}

void ExtendedRewriter::debugExtendedRewrite( Node n, Node ret, const char * c )
{
  Trace("q-ext-rewrite") << "sygus-extr : " << n << " rewrites to "
                          << ret << " due to " <<  c << "." << std::endl;
}

} /* CVC4::theory::quantifiers namespace */
} /* CVC4::theory namespace */
} /* CVC4 namespace */
