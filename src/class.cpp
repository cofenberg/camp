/****************************************************************************
**
** This file is part of the CAMP library.
**
** The MIT License (MIT)
**
** Copyright (C) 2009-2014 TEGESO/TEGESOFT and/or its subsidiary(-ies) and mother company.
** Contact: Tegesoft Information (contact@tegesoft.com)
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
**
****************************************************************************/


#include <camp/class.hpp>
#include <camp/constructor.hpp>


namespace camp
{
//-------------------------------------------------------------------------------------------------
uint32_t Class::id() const
{
    return m_id;
}

//-------------------------------------------------------------------------------------------------
const char* Class::name() const
{
    return m_name;
}

//-------------------------------------------------------------------------------------------------
std::size_t Class::baseCount() const
{
    return m_bases.size();
}

//-------------------------------------------------------------------------------------------------
const Class& Class::base(std::size_t index) const
{
    // Make sure that the index is not out of range
    if (index >= m_bases.size())
        CAMP_ERROR(OutOfRange(index, m_bases.size()));

    return *m_bases[index].base;
}

//-------------------------------------------------------------------------------------------------
std::size_t Class::functionCount() const
{
    return m_functions.size();
}

//-------------------------------------------------------------------------------------------------
bool Class::hasFunction(StringId id) const
{
    SortedFunctionVector::const_iterator iterator = std::lower_bound(m_functions.cbegin(), m_functions.cend(), id, OrderByFunctionId());
    return (iterator != m_functions.end() && iterator->id == id);
}

//-------------------------------------------------------------------------------------------------
const Function& Class::getFunctionByIndex(std::size_t index) const
{
    // Make sure that the index is not out of range
    if (index >= m_functions.size())
        CAMP_ERROR(OutOfRange(index, m_functions.size()));

    return *m_functions[index].functionPtr;
}

//-------------------------------------------------------------------------------------------------
const Function& Class::getFunctionById(StringId id) const
{
    SortedFunctionVector::const_iterator iterator = std::lower_bound(m_functions.cbegin(), m_functions.cend(), id, OrderByFunctionId());
    if (iterator != m_functions.end() && iterator->id == id)
    {
        // Found
        return *iterator->functionPtr;
    }
    else
    {
        // Not found
        CAMP_ERROR(FunctionNotFound(id, m_name));
    }
}

//-------------------------------------------------------------------------------------------------
const Function* Class::tryGetFunctionById(StringId id) const
{
    SortedFunctionVector::const_iterator iterator = std::lower_bound(m_functions.cbegin(), m_functions.cend(), id, OrderByFunctionId());
    return (iterator != m_functions.end() && iterator->id == id) ? iterator->functionPtr.get() : nullptr;
}

//-------------------------------------------------------------------------------------------------
std::size_t Class::propertyCount() const
{
    return m_propertiesById.size();
}

//-------------------------------------------------------------------------------------------------
bool Class::hasProperty(StringId id) const
{
    SortedPropertyVector::const_iterator iterator = std::lower_bound(m_propertiesById.cbegin(), m_propertiesById.cend(), id, OrderByPropertyId());
    return (iterator != m_propertiesById.end() && iterator->id == id);
}

//-------------------------------------------------------------------------------------------------
const Property& Class::getPropertyByIndex(std::size_t index) const
{
    // Make sure that the index is not out of range
    if (index >= m_propertiesById.size())
        CAMP_ERROR(OutOfRange(index, m_propertiesById.size()));

    return *m_propertiesByIndex[index].propertyPtr;
}

//-------------------------------------------------------------------------------------------------
const Property& Class::getPropertyById(StringId id) const
{
    SortedPropertyVector::const_iterator iterator = std::lower_bound(m_propertiesById.cbegin(), m_propertiesById.cend(), id, OrderByPropertyId());
    if (iterator != m_propertiesById.end() && iterator->id == id)
    {
        // Found
        return *iterator->propertyPtr;
    }
    else
    {
        // Not found
        CAMP_ERROR(PropertyNotFound(id, m_name));
    }
}

//-------------------------------------------------------------------------------------------------
const Property* Class::tryGetPropertyById(StringId id) const
{
    SortedPropertyVector::const_iterator iterator = std::lower_bound(m_propertiesById.cbegin(), m_propertiesById.cend(), id, OrderByPropertyId());
    return (iterator != m_propertiesById.end() && iterator->id == id) ? iterator->propertyPtr.get() : nullptr;
}

//-------------------------------------------------------------------------------------------------
std::size_t Class::constructorCount() const
{
    return m_constructors.size();
}

//-------------------------------------------------------------------------------------------------
UserObject Class::construct(const Args& args) const
{
    // Search an arguments match among the list of available constructors
    const size_t numberOfConstructors = m_constructors.size();
    for (size_t i = 0; i < numberOfConstructors; ++i)
    {
        const Constructor* constructor = m_constructors[i];
        if (constructor->matches(args))
        {
            // Match found: use the constructor to create the new instance
            return constructor->create(args);
        }
    }

    // No match found
    return UserObject::nothing;
}

//-------------------------------------------------------------------------------------------------
void Class::destroy(const UserObject& object) const
{
    m_destructor(object);
}

//-------------------------------------------------------------------------------------------------
void Class::visit(ClassVisitor& visitor) const
{
    { // First visit properties
        const size_t numberOfProperties = m_propertiesById.size();
        for (size_t i = 0; i < numberOfProperties; ++i)
        {
            m_propertiesById[i].propertyPtr->accept(visitor);
        }
    }

    { // Then visit functions
        const size_t numberOfFunctions = m_functions.size();
        for (size_t i = 0; i < numberOfFunctions; ++i)
        {
            m_functions[i].functionPtr->accept(visitor);
        }
    }
}

//-------------------------------------------------------------------------------------------------
void* Class::applyOffset(void* pointer, const Class& target) const
{
    // Special case for null pointers: don't apply offset to leave them null
    if (!pointer)
        return pointer;

    // Check target as a base class of this
    int offset = baseOffset(target);
    if (offset != -1)
        return static_cast<void*>(static_cast<char*>(pointer) + offset);

    // Check target as a derived class of this
    offset = target.baseOffset(*this);
    if (offset != -1)
        return static_cast<void*>(static_cast<char*>(pointer) - offset);

    // No match found, target is not a base class nor a derived class of this
    CAMP_ERROR(ClassUnrelated(name(), target.name()));
}

//-------------------------------------------------------------------------------------------------
bool Class::operator==(const Class& other) const
{
    return m_id == other.m_id;
}

//-------------------------------------------------------------------------------------------------
bool Class::operator!=(const Class& other) const
{
    return m_id != other.m_id;
}

//-------------------------------------------------------------------------------------------------
Class::Class(StringId id, const char* name)
    : m_id(id)
    , m_name(name)
{
}

//-------------------------------------------------------------------------------------------------
Class::~Class()
{
    const size_t numberOfConstructors = m_constructors.size();
    for (size_t i = 0; i < numberOfConstructors; ++i)
    {
        delete m_constructors[i];
    }
}

//-------------------------------------------------------------------------------------------------
int Class::baseOffset(const Class& base) const
{
    // Check self
    if (&base == this)
    {
        return 0;
    }

    // Search base in the base classes
    const size_t numberOfBases = m_bases.size();
    for (size_t i = 0; i < numberOfBases; ++i)
    {
        const BaseInfo& baseInfo = m_bases[i];
        const int offset = baseInfo.base->baseOffset(base);
        if (offset != -1)
        {
            return offset + baseInfo.offset;
        }
    }

    return -1;
}

} // namespace camp
