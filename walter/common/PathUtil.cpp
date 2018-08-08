// Copyright 2017 Rodeo FX.  All rights reserved.

#include "PathUtil.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm/count_if.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

/**
 * @brief Erase everything in a string between two symbols.
 *
 * @param ioStr Input-output string.
 * @param iOpen The open bracket.
 * @param iClose The close bracket.
 */
void eraseBrackets(std::string& ioStr, char iOpen, char iClose)
{
    size_t foundOpen = 0;
    while (true)
    {
        size_t foundOpen = ioStr.find(iOpen);
        if (foundOpen == std::string::npos)
        {
            break;
        }

        size_t foundClose = ioStr.find(iClose, foundOpen);
        if (foundClose == std::string::npos)
        {
            break;
        }

        ioStr.erase(foundOpen, foundClose - foundOpen);
    }
}

bool WalterCommon::matchPattern(const std::string& str, const std::string& pat)
{
    void* rx = createRegex(pat);
    bool result = searchRegex(rx, str);
    clearRegex(rx);
    return result;
}

void* WalterCommon::createRegex(const std::string& exp)
{
    return new boost::regex(exp);
}

bool WalterCommon::searchRegex(void* regex, const std::string& str)
{
    return boost::regex_match(str, *reinterpret_cast<boost::regex*>(regex));
}

std::string WalterCommon::mangleString(const std::string& expression)
{
    return boost::replace_all_copy(expression, "/", "\\");
}

std::string WalterCommon::demangleString(const std::string& expression)
{
    return boost::replace_all_copy(expression, "\\", "/");
}

std::string WalterCommon::convertRegex(const std::string& expression)
{
    std::string expr = boost::replace_all_copy(expression, "\\d", "[0-9]");
    boost::replace_all(expr, "\\D", "[^0-9]");
    boost::replace_all(expr, "\\w", "[a-zA-Z0-9_]");
    boost::replace_all(expr, "\\W", "[^a-zA-Z0-9_]");
    return expr;
}

void WalterCommon::clearRegex(void* regex)
{
    delete reinterpret_cast<boost::regex*>(regex);
}

WalterCommon::Expression::Expression(const std::string& iExpression) :
        mExpression(iExpression),
        mMinSize(mExpression.size())
{
    // The list of characters that characterize a regex.
    static const char* sExpChars = ".*+|<>&-[](){}?$^\\";

    // We need to detect if it's an expression to be able to skip expensive
    // matching stuff.
    if (boost::find_token(mExpression, boost::is_any_of(sExpChars)))
    {
        mRegex = createRegex(mExpression);

        // Compute the minimal number of characters in the object name and the
        // text of the expression without regex special symbols. It can be a
        // good filter.
        mTextWithoutRegex = mExpression;
        // Don't consider anything inside brackets.
        eraseBrackets(mTextWithoutRegex, '[', ']');
        eraseBrackets(mTextWithoutRegex, '(', ')');
        eraseBrackets(mTextWithoutRegex, '{', '}');
        mTextWithoutRegex.erase(
            std::remove_if(
                mTextWithoutRegex.begin(),
                mTextWithoutRegex.end(),
                boost::is_any_of(sExpChars)),
            mTextWithoutRegex.end());

        // Don't consider anything from sExpChars.
        mMinSize = mTextWithoutRegex.size();
    }
    else
    {
        mRegex = nullptr;
    }
}

WalterCommon::Expression::~Expression()
{
    if (mRegex)
    {
        clearRegex(mRegex);
    }
}

bool WalterCommon::Expression::isParentOf(
    const std::string& iFullName,
    bool* oIsItself) const
{
    if (isRegex() || iFullName.size() < mMinSize)
    {
        // Filter them out.
        return false;
    }

    if (boost::starts_with(iFullName, mExpression))
    {
        if (iFullName.size() == mMinSize)
        {
            // Two strings exactly match.
            if (oIsItself)
            {
                *oIsItself = true;
            }

            return true;
        }

        // We are here because the length of iFullName is more than mMinSize. We
        // need to check that it's a parent and  we don't have a case like this:
        // "/Hello" "/HelloWorld".
        return iFullName[mMinSize] == '/';
    }

    return false;
}

bool WalterCommon::Expression::matchesPath(const std::string& iFullName) const
{
    if (!isRegex() || iFullName.size() < mMinSize)
    {
        // Filter them out.
        return false;
    }

    return searchRegex(mRegex, iFullName);
}

bool WalterCommon::Expression::matchesPath(
    const WalterCommon::Expression& iExpression) const
{
    if (!isRegex())
    {
        // Filter them out.
        return false;
    }

    if (!iExpression.isRegex())
    {
        // The inpot is a string, we can match is as a string.
        return matchesPath(iExpression.getExpression());
    }

    // We need to match it with something left from the expression.
    return matchesPath(iExpression.mTextWithoutRegex);
}
