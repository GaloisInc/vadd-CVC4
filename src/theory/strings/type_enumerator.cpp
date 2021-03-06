/*********************                                                        */
/*! \file type_enumerator.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Reynolds
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Implementation of enumerators for strings
 **/

#include "theory/strings/type_enumerator.h"

#include "theory/strings/theory_strings_utils.h"
#include "util/regexp.h"

namespace CVC4 {
namespace theory {
namespace strings {

WordIter::WordIter(uint32_t startLength) : d_hasEndLength(false), d_endLength(0)
{
  for (uint32_t i = 0; i < startLength; i++)
  {
    d_data.push_back(0);
  }
}
WordIter::WordIter(uint32_t startLength, uint32_t endLength)
    : d_hasEndLength(true), d_endLength(endLength)
{
  for (uint32_t i = 0; i < startLength; i++)
  {
    d_data.push_back(0);
  }
}

WordIter::WordIter(const WordIter& witer)
    : d_hasEndLength(witer.d_hasEndLength),
      d_endLength(witer.d_endLength),
      d_data(witer.d_data)
{
}

const std::vector<unsigned>& WordIter::getData() const { return d_data; }

bool WordIter::increment(uint32_t card)
{
  for (unsigned i = 0, dsize = d_data.size(); i < dsize; ++i)
  {
    if (d_data[i] + 1 < card)
    {
      ++d_data[i];
      return true;
    }
    else
    {
      d_data[i] = 0;
    }
  }
  if (d_hasEndLength && d_data.size() == d_endLength)
  {
    return false;
  }
  // otherwise increase length
  d_data.push_back(0);
  return true;
}

SEnumLen::SEnumLen(TypeNode tn, uint32_t startLength)
    : d_type(tn), d_witer(new WordIter(startLength))
{
}
SEnumLen::SEnumLen(TypeNode tn, uint32_t startLength, uint32_t endLength)
    : d_type(tn), d_witer(new WordIter(startLength, endLength))
{
}

SEnumLen::SEnumLen(const SEnumLen& e)
    : d_type(e.d_type), d_witer(new WordIter(*e.d_witer)), d_curr(e.d_curr)
{
}

Node SEnumLen::getCurrent() const { return d_curr; }

bool SEnumLen::isFinished() const { return d_curr.isNull(); }

StringEnumLen::StringEnumLen(uint32_t startLength,
                             uint32_t endLength,
                             uint32_t card)
    : SEnumLen(NodeManager::currentNM()->stringType(), startLength, endLength),
      d_cardinality(card)
{
  mkCurr();
}

StringEnumLen::StringEnumLen(uint32_t startLength, uint32_t card)
    : SEnumLen(NodeManager::currentNM()->stringType(), startLength),
      d_cardinality(card)
{
  mkCurr();
}

bool StringEnumLen::increment()
{
  // always use the same cardinality
  if (!d_witer->increment(d_cardinality))
  {
    d_curr = Node::null();
    return false;
  }
  mkCurr();
  return true;
}

void StringEnumLen::mkCurr()
{
  d_curr = NodeManager::currentNM()->mkConst(String(d_witer->getData()));
}

StringEnumerator::StringEnumerator(TypeNode type, TypeEnumeratorProperties* tep)
    : TypeEnumeratorBase<StringEnumerator>(type),
      d_wenum(0, utils::getAlphabetCardinality())
{
  Assert(type.getKind() == kind::TYPE_CONSTANT
         && type.getConst<TypeConstant>() == STRING_TYPE);
}

StringEnumerator::StringEnumerator(const StringEnumerator& enumerator)
    : TypeEnumeratorBase<StringEnumerator>(enumerator.getType()),
      d_wenum(enumerator.d_wenum)
{
}

Node StringEnumerator::operator*() { return d_wenum.getCurrent(); }

StringEnumerator& StringEnumerator::operator++()
{
  d_wenum.increment();
  return *this;
}

bool StringEnumerator::isFinished() { return d_wenum.isFinished(); }

}  // namespace strings
}  // namespace theory
}  // namespace CVC4
