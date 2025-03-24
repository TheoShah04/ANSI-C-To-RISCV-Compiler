#pragma once

#include <string_view>
#include <stdexcept>
#include <unordered_map>
#include <map>

namespace ast {


enum class TypeSpecifier
{
    INT,
    FLOAT,
    CHAR,
    DOUBLE,
    VOID,
    ENUM,
    UNSIGNED,
    SIGNED,
    CUSTOM,
    LFLOAT,
    LDOUBLE,
    SCHAR,
    UCHAR,
    SINT,
    UINT,
    LINT,
    SHINT,
    USHINT,
    SSHINT,
    ULINT,
    SLINT,
    CHARPTR
};

template<typename LogStream>
LogStream& operator<<(LogStream& ls, const TypeSpecifier& type)
{
    const auto TypeToString = [](TypeSpecifier type) -> std::string {
        static const std::map<ast::TypeSpecifier, std::string> type_map = {
            {TypeSpecifier::INT, "int"},
            {TypeSpecifier::FLOAT, "float"},
            {TypeSpecifier::CHAR, "char"},
            {TypeSpecifier::DOUBLE, "double"},
            {TypeSpecifier::VOID, "void"},
            {TypeSpecifier::UNSIGNED, "unsigned"},
            {TypeSpecifier::SIGNED, "signed"},
            {TypeSpecifier::CUSTOM, "custom"},
            {TypeSpecifier::LFLOAT, "lfloat"},
            {TypeSpecifier::LDOUBLE, "ldouble"},
            {TypeSpecifier::SCHAR, "schar"},
            {TypeSpecifier::UCHAR, "uchar"},
            {TypeSpecifier::SINT, "sint"},
            {TypeSpecifier::UINT, "uint" },
            {TypeSpecifier::LINT, "lint"},
            {TypeSpecifier::SHINT, "shint"},
            {TypeSpecifier::USHINT, "ushint"},
            {TypeSpecifier::SSHINT, "sshint"},
            {TypeSpecifier::ULINT, "ulint"},
            {TypeSpecifier::SLINT, "slint"},
            {TypeSpecifier::CHARPTR, "string"}
        };

        auto it = type_map.find(type);
        if(it != type_map.end())
            return it->second;

        throw std::runtime_error("Unexpected type specifier");
    };
    return ls << TypeToString(type);
}


class TypePointer {
    private:
        TypeSpecifier baseType;
        bool isPointerType;
        unsigned int pointerLevel; // pointer to pointer

    public:
        TypePointer(TypeSpecifier base, bool isPtr = false, unsigned int ptrLevel = 0)
            : baseType(base), isPointerType(isPtr), pointerLevel(ptrLevel) {}

        TypeSpecifier getBaseType() const { return baseType; }
        bool isPointer() const { return isPointerType; }
        unsigned int getPointerLevel() const { return pointerLevel; }

        TypePointer getPointeeType() const {
            return TypePointer(baseType, pointerLevel > 1, pointerLevel - 1);
        }
    };

class EnumType {
private:
    std::string name;
    std::unordered_map<std::string, int> values;

public:
    EnumType(const std::string& name = "") : name(name) {}

    void addValue(const std::string& valueName, int value) {values[valueName] = value;}
    bool hasValue(const std::string& valueName) const {return values.find(valueName) != values.end();}

    int getValue(const std::string& valueName) const {
        auto it = values.find(valueName);
        if (it != values.end()) {
            return it->second;
        }
    }

    const std::string& getName() const { return name; }
    const std::unordered_map<std::string, int>& getValues() const { return values; }
};

}
