/*
 * SQLassie - database firewall
 * Copyright (C) 2011 Brandon Skari <brandon.skari@gmail.com>
 *
 * This file is part of SQLassie.
 *
 * SQLassie is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SQLassie is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SQLassie. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ExpressionNode.hpp"
#include "SensitiveNameChecker.hpp"
#include "InValuesListNode.hpp"
#include "nullptr.hpp"
#include "QueryRisk.hpp"

#include <boost/cast.hpp>
#include <boost/regex.hpp>
#include <cassert>
#include <string>
#include <vector>

using boost::polymorphic_downcast;
using boost::regex;
using boost::regex_match;
using std::string;
using std::vector;


InValuesListNode::InValuesListNode(const ExpressionNode* const expression)
    : ExpressionNode("InValuesList")
    , expression_(expression)
{
}


InValuesListNode::~InValuesListNode()
{
    delete expression_;
}


AstNode* InValuesListNode::copy() const
{
    InValuesListNode* const temp = new InValuesListNode(expression_);
    AstNode::addCopyOfChildren(temp);
    return temp;
}


bool InValuesListNode::isAlwaysTrueOrFalse() const
{
    if (!expression_->resultsInValue() || !expression_->resultsInString())
    {
        return false;
    }

    // Everything else needs to result in a value or string
    vector<const AstNode*>::const_iterator end(children_.end());
    for (
        vector<const AstNode*>::const_iterator i(children_.begin());
        i != end;
        ++i
    )
    {
        const ExpressionNode* const expr =
            polymorphic_downcast<const ExpressionNode*>(*i);

        if (!expr->resultsInValue() || !expr->resultsInString())
        {
            return false;
        }
    }
    return true;
}


bool InValuesListNode::isAlwaysTrue() const
{
    bool in = false;

    // Identifiers may or may not be in a list - it's legitimate
    const bool isValue = expression_->resultsInValue();
    const bool isString = expression_->resultsInString();
    if (!isValue && !isString)
    {
        return false;
    }

    const string firstExpression(expression_->getValue());

    vector<const AstNode*>::const_iterator end(children_.end());
    for (
        vector<const AstNode*>::const_iterator i(children_.begin());
        i != end;
        ++i
    )
    {
        const ExpressionNode* const expr =
            polymorphic_downcast<const ExpressionNode*>(*i);

        if (
            (
                isValue == expr->resultsInValue()
                || expr->resultsInString()
            )
            && firstExpression == expr->getValue()
        )
        {
            in = true;
            break;
        }
    }
    return in;
}


bool InValuesListNode::anyIsAlwaysTrue() const
{
    return isAlwaysTrue();
}


QueryRisk::EmptyPassword InValuesListNode::emptyPassword() const
{
    // If we're checking something like "password IN (...)", something is fishy
    // This isn't quite as severe as "password = ''", but let's err on the side
    // of caution and treat it as such anyway
    /// @TODO(bskari): Come up with a more accurate way of handling this
    if (SensitiveNameChecker::get().isPasswordField(expression_->getValue()))
    {
        return QueryRisk::PASSWORD_EMPTY;
    }
    return QueryRisk::PASSWORD_NOT_USED;
}


bool InValuesListNode::resultsInValue() const
{
    return expression_->resultsInValue();
}


string InValuesListNode::getValue() const
{
    assert(resultsInValue());

    if (isAlwaysTrue())
    {
        return "1";
    }
    return "0";
}


void InValuesListNode::print(
    std::ostream& out,
    const int depth,
    const char indent
) const
{
    for (int i = 0; i < depth; ++i)
    {
        out << indent;
    }
    out << name_ << ":\n{\n" << *expression_;

    for (int i = 0; i < depth; ++i)
    {
        out << indent;
    }
    out << "In\n";

    printChildren(out, depth + 1, indent);
    for (int i = 0; i < depth; ++i)
    {
        out << indent;
    }
    out << "}\n";
}
