// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERCOMMONPATHUTIL_H_
#define __WALTERCOMMONPATHUTIL_H_

#include <boost/noncopyable.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <map>

namespace WalterCommon
{
/**
 * @brief Checks if regex matches string.
 *
 * @param str Regex
 * @param pat String
 *
 * @return True if matches.
 */
bool matchPattern(const std::string& str, const std::string& pat);

/**
 * @brief Creates regex object. It's `void*` to be able to use this function
 * without including boost::regex stuff. It's the responsibility of the
 * programmer to remove this object with `clearRegex`. Right now the functions
 * `createRegex` and `searchRegex` are wraps on `boost::regex` and
 * `boost::regex_match`.
 *
 * @param exp Regex string.
 *
 * @return A pointer to the regex object.
 */
void* createRegex(const std::string& exp);

/**
 * @brief Checks if regex matches string. Right now the functions `createRegex`
 * and `searchRegex` are wraps on `boost::regex` and `boost::regex_match`.
 *
 * @param regex The poiter created with `matchPattern`.
 * @param str The string to check.
 *
 * @return True if matches.
 */
bool searchRegex(void* regex, const std::string& str);

/**
 * @brief Mangles the string to replace '/' by '\'. It's because Alembic doesn't
 * allow to use '/' in an attribute name so we need to replace it by '\' and
 * replace it back when we read it (cf. demangleString).
 *
 * @param str The string to mangle
 *
 * @return The mangled string.
 */
std::string mangleString(const std::string& str);

/**
 * @brief Demangles the string to replace '\' by '/'.
 *
 * @param str The string to demangle
 *
 * @return The demangled string.
 */
std::string demangleString(const std::string& str);

/**
 * @brief Replace special regex symbols using a backslash (e.g '\d')
 * so the regex expression is not broken when converted back and
 * forth between different 'modules' (Walter, UDS, Abc, ...) that
 * converts slashs to backslashs and conversely.
 *
 * @param expression The regex expression to convert
 *
 * @return The converted expression.
 */
std::string convertRegex(const std::string& expression);

/**
 * @brief Removes regex created with `matchPattern`.
 *
 * @param regex The poiter created with `matchPattern`.
 */
void clearRegex(void* regex);

class Expression : private boost::noncopyable
{
public:
    explicit Expression(const std::string& iExpression);
    ~Expression();

    /**
     * @brief Checks if this is a full path and if it's a parent of the provided
     * path.
     *
     * @param iFullName The full name of an object to check.
     * @param oIsItself It's set to true if iFullName matches to this.
     *
     * @return True of it's a parent or it it's the same object.
     */
    bool isParentOf(const std::string& iFullName, bool* oIsItself) const;

    /**
     * @brief Checks if this is the regex and if it matches the provided path.
     *
     * @param iFullName The full name of an object to check.
     *
     * @return True if matches.
     */
    bool matchesPath(const std::string& iFullName) const;

    /**
     * @brief Checks if this is the regex and if it matches another one.
     *
     * @param iExpression The another expression to match.
     *
     * @return True if matches.
     */
    bool matchesPath(const Expression& iExpression) const;

    /**
     * @brief Return the expression or object name.
     *
     * @return The reference to the std::string object.
     */
    const std::string& getExpression() const { return mExpression; }

    /**
     * @brief Checks if this object is considered as regex.
     *
     * @return True if this object contains symbols of regex.
     */
    const bool isRegex() const { return mRegex; }

    // std::map routines that allow using this object as a key.
    bool operator<(const Expression& rhs) const
    {
        return mExpression < rhs.mExpression;
    }

    bool operator==(const Expression& rhs) const
    {
        return mExpression == rhs.mExpression;
    }

private:
    // The expression or full name.
    std::string mExpression;

    // The regex cache.
    void* mRegex;

    // The min size the path should have to be comparable. We use it as a filter
    // to prevent comparison of the objects that will not match.
    size_t mMinSize;

    // The string with no regex characters. We use it to detect the similar
    // expressions.
    std::string mTextWithoutRegex;
};

/**
 * @brief Check the map with all the assignments and return the assignment that
 * closely fits the specified object name. It should work with following
 * priority:
 *  1) Exact name
 *  2) Exact expression match
 *  3) Closest parent
 *
 * @param iFullName Full object name
 * @param iExpressions All the assignments in sorted map.
 *
 * @return The pointer to the second element of the map.
 */
template <class T>
T const* resolveAssignment(
    const std::string& iFullName,
    const std::map<Expression, T>& iExpressions)
{
    T const* shader = nullptr;

    // Since AssignmentLayers is std::map, it's already sorted.
    // boost::adaptors::reverse wraps an existing range to provide a new range
    // whose iterators behave as if they were wrapped in reverse_iterator. We
    // need it reversed to be sure we meet the parent close to the object before
    // the parent close to the root.
    for (const auto& object : boost::adaptors::reverse(iExpressions))
    {
        const Expression& expr = object.first;

        bool isItself = false;
        if (!shader && expr.isParentOf(iFullName, &isItself))
        {
            // We don't return if we found a parent because we can find
            // expression that matches this object. The expression has higher
            // priority than the parent object.
            shader = &object.second;

            if (isItself)
            {
                // It's the exact match.
                return shader;
            }
        }

        if (expr.matchesPath(iFullName))
        {
            // It's the expression that matches the name. Return.
            return &object.second;
        }
    }

    return shader;
}
}

#endif
