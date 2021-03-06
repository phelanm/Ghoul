/*****************************************************************************************
 *                                                                                       *
 * GHOUL                                                                                 *
 * General Helpful Open Utility Library                                                  *
 *                                                                                       *
 * Copyright (c) 2012-2016                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#ifndef __BOOLEAN_H__
#define __BOOLEAN_H__

namespace ghoul {

/**
 * This structure can be used to make a more expressive boolean parameter for methods.
 * Instead of using <code>bool</code>, which does not convey any direct meaning and forces
 * the programmer to read the documentation what the parameter does, this class can be
 * <code>typedef</code>ed to a meaningful name.
 *
 * For example:
 * \verbatim
using AllowOverride = ghoul::Boolean;
 
void foo(std::string value, AllowOverride override = AllowOverride::Yes);
\endverbatim
 * Though it is more verbal, it elimiates ambiguity regarding parameters. This class is
 * implicitly convertible to <code>bool</code>, but not the other way around. Furthermore,
 * it supports the <code>==</code>, <code>!=</code>, and <code>!</code> operators.
 */
struct Boolean {
    enum Value {
        Yes = 1,
        No = 0
    };
   
    // Non-explicit constructor so that we c
    Boolean(Value v) : value(v) {}
    
    Value value;
    
    /**
     * This operator returns <code>true</code> if the stored value is equal to
     * <code>Yes</code>.
     */
    operator bool() { return value == Yes; };
    operator bool() const { return value == Yes; };
};
    
} // namespace ghoul

#endif // __BOOLEAN_H__
