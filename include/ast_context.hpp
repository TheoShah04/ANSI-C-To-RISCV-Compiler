#pragma once
#include "ast_type_specifier.hpp"

#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include <stack>
#include <optional>
#include <stdexcept>
#include <ostream>
#include <iostream>
#include <stdint.h>

namespace ast {

constexpr int TOTAL_STACK_SIZE = 1024;
constexpr int POINTER_MEM = 4;

extern const std::unordered_map<TypeSpecifier, unsigned int> TYPE_SIZE;

struct Variable {
    int stack_offset;
    TypeSpecifier type;
    bool is_parameter;
    bool is_pointer;
    bool is_array;
    int array_size;
    TypeSpecifier pointeeType;
    bool is_stack_param;

    // map requires default-constructible values
    Variable() : stack_offset(0), type(TypeSpecifier::INT),is_parameter(false), is_pointer(false),
                is_array(0), array_size(0), pointeeType(TypeSpecifier::VOID), is_stack_param(false) {}

    Variable(int offset, TypeSpecifier t, bool param = false, bool ptr = false, TypeSpecifier pointee = TypeSpecifier::VOID)
        : stack_offset(offset), type(t), is_parameter(param), is_pointer(ptr),
            is_array(false), array_size(0), pointeeType(pointee), is_stack_param(false) {}
};

struct PointerTypes {
    TypeSpecifier baseType;
    bool isPointer;
    int pointerLevel;

    PointerTypes(TypeSpecifier type, bool ptr = false, int level = 0)
        : baseType(type), isPointer(ptr), pointerLevel(level) {}
};

class Context
{
private:

    const std::unordered_map<TypeSpecifier, unsigned int> TYPE_SIZE = {
        {TypeSpecifier::VOID,   0},
        {TypeSpecifier::CHAR,   1},
        {TypeSpecifier::INT,    4},
        {TypeSpecifier::FLOAT,  4},
        {TypeSpecifier::DOUBLE, 8},
    };

    //scope management
    using Scope = std::unordered_map<std::string, Variable>;
    std::vector<Scope> scopes;
    std::vector<bool> function_scopes;
    std::vector<std::string> current_function_stack;
    std::unordered_map<std::string, bool> global_variables; // true if var is global

    //stack management
    int stack_offset;
    int used_stack_memory;
    std::vector<int> remaining_mem_stack;

    std::unordered_map<std::string, TypeSpecifier> function_return_types;
    std::unordered_map<std::string, EnumType> enumTypes;
    std::unordered_map<std::string, std::pair<std::string, int>> enumValues;

    std::unordered_map<std::string, std::string> function_end_labels;
    std::unordered_map<float, std::string> float_labels; // map float values to data section
    std::unordered_map<double, std::string> double_labels; // map double values to data section
    std::unordered_map<std::string, std::string> string_labels; // map double values to data section
    std::vector<std::string> break_targets;
    std::string current_switch_reg;
    std::vector<std::string> continue_targets;

    std::vector<std::vector<Variable>> parameters_stack;

    std::set<std::string> used_registers;
    std::set<std::string> used_float_registers;
    std::unordered_map<std::string, int> saved_registers;
    std::unordered_map<std::string, int> saved_float_registers;

    std::vector<float> floatValues;
    std::vector<double> doubleValues;
    std::vector<std::string> stringValues;

    std::unordered_map<std::string, int> arraySize;

    int last_stack_adjust;

    int label_counter;

    TypeSpecifier current_declaration_type;

    int getMemory(int mem_size) {
        if (mem_size % 4 != 0) {
            mem_size = ((mem_size + 3) / 4) * 4;  // Align to 4 bytes
        }

        if (used_stack_memory + mem_size > TOTAL_STACK_SIZE) {
            throw std::runtime_error("Stack overflow");
        }
        used_stack_memory += mem_size;
        int offset = used_stack_memory - mem_size;
        return offset;
    }

public:
    Context()
        : stack_offset(0)
        , used_stack_memory(0)
        , label_counter(0)
        , current_declaration_type(TypeSpecifier::INT)
    {
        scopes.push_back(Scope());
        function_scopes.push_back(false);
    }

    void enterScope(bool isFunction) {
        scopes.push_back(Scope());
        parameters_stack.push_back(std::vector<Variable>());
        remaining_mem_stack.push_back(TOTAL_STACK_SIZE - used_stack_memory);

        if (isFunction) {
            function_scopes.push_back(true);
            stack_offset = 0;
            used_stack_memory = 0;
        } else {
            function_scopes.push_back(false);
        }
    }

    void exitScope() {
        if (scopes.size() <= 1) {
            throw std::runtime_error("Global Scope: CANNOT EXIT");
        }

        std::vector<Variable> params = parameters_stack.back();
        parameters_stack.pop_back();

        //int remaining_mem = remaining_mem_stack.back();
        remaining_mem_stack.pop_back();

        if (!function_scopes.empty() && function_scopes.back()) {
            if (!current_function_stack.empty()) {
                current_function_stack.pop_back();
            }
        }
        function_scopes.pop_back();
        scopes.pop_back();
    }

    int ScopeDepth() const {
        return scopes.size() - 1;
    }

    void setFunctionReturnType(const std::string& function_name, TypeSpecifier return_type, bool isPointer) {
        (void)isPointer;
        function_return_types[function_name] = return_type;
    }

    TypeSpecifier getFunctionReturnType(const std::string& function_name) const {
        auto it = function_return_types.find(function_name);
        if (it != function_return_types.end()) {
            return it->second;
        }
        throw std::runtime_error("Unknown function: " + function_name);
    }

    bool functionExists(const std::string& function_name) const {
        return function_return_types.find(function_name) != function_return_types.end();
    }

    void beginFunction(std::ostream& stream, const std::string& name, TypeSpecifier return_type, bool isPointer) {
        setFunctionReturnType(name, return_type, isPointer);

        std::string end_label = generateUniqueLabel("func_end");
        function_end_labels[name] = end_label;

        current_function_stack.push_back(name);

        enterScope(true);
        stream << "    addi sp, sp, -" << TOTAL_STACK_SIZE << std::endl;
        stream << "    sw ra, " << (TOTAL_STACK_SIZE - 4) << "(sp)" << std::endl;
        stream << "    sw s0, " << (TOTAL_STACK_SIZE - 8) << "(sp)" << std::endl;
        stream << "    mv s0, sp" << std::endl;
    }

    void endFunction(std::ostream& stream, const std::string& name) {
        if (!functionExists(name)) {
            throw std::runtime_error("Not in a function: " + name);
        }

        stream << function_end_labels[name] << ":" << std::endl;

        stream << "    mv sp, s0" << std::endl;
        stream << "    lw s0, " << (TOTAL_STACK_SIZE - 8) << "(sp)" << std::endl;
        stream << "    lw ra, " << (TOTAL_STACK_SIZE - 4) << "(sp)" << std::endl;
        stream << "    addi sp, sp, " << TOTAL_STACK_SIZE << std::endl;
        stream << "    jr ra" << std::endl;
        exitScope();
    }

    const std::set<std::string>& getUsedRegisters() const {
        return used_registers;
    }

    std::string getCurrentFunction() const {
        if (current_function_stack.empty()) {
            throw std::runtime_error("Not in a function");
        }
        return current_function_stack.back();
    }

    std::string getFunctionEndLabel(const std::string& function_name) const {
        auto it = function_end_labels.find(function_name);
        if (it != function_end_labels.end()) {
            return it->second;
        }
        throw std::runtime_error("Unknown function end label: " + function_name);
    }

    void storeFloatValue(float value){
        floatValues.push_back(value);
    }

    void printFloatData(std::ostream& stream){
        for(const auto& [floatValue, label] : float_labels){
            stream << label << ":" << std::endl;
            uint32_t bits = *(uint32_t*)&floatValue;
            stream << "    .word " << bits << std::endl;
        }
    }

    void storeDoubleValue(double value){
        doubleValues.push_back(value);
    }

    void printDoubleData(std::ostream& stream){
        stream << "    .section    .rodata" << std::endl;
        for(const auto& [doubleValue, label] : double_labels){
            stream << label << ":" << std::endl;
            union {
                double d;
                uint32_t parts[2];
            } doubleUnion;
            doubleUnion.d = doubleValue;
            stream << "    .word " << doubleUnion.parts[0] << std::endl;
            stream << "    .word " << doubleUnion.parts[1] << std::endl;
        }
    }

    void storeStringValue(std::string value){
        stringValues.push_back(value);
    }

    void printStringData(std::ostream& stream){
        for(const auto& [stringValue, label] : string_labels){
            stream << label << ":" << std::endl;
            stream << "    .string " << stringValue << std::endl;
        }
    }

    Variable declareVariable(const std::string& id, TypeSpecifier type, bool isPointer = false,
         TypeSpecifier pointeeType = TypeSpecifier::VOID) {
        if (scopes.back().find(id) != scopes.back().end()) {
            throw std::runtime_error("Variable '" + id + "' already declared in current scope");
        }

        unsigned int mem_size = getTypeSize(type);
        if (isPointer) {
            mem_size = POINTER_MEM;
        }

        int offset = getMemory(mem_size);
        std::cerr << "DEBUG: New variable" << std::endl;
        Variable newVar(offset, type, false, isPointer, pointeeType);
        scopes.back()[id] = newVar;
        return newVar;
    }

    Variable declareArray(const std::string& id, TypeSpecifier type, int arraySize) {
        if (scopes.back().find(id) != scopes.back().end()) {
            throw std::runtime_error("Variable '" + id + "' already declared in current scope");
        }
        unsigned int elementSize = getTypeSize(type);
        unsigned int totalMem = elementSize*arraySize;

        // align to 4 bytes (int) -> maybe could put a .align assembler directive here too
        if(totalMem % 4 != 0) {
            totalMem = ((totalMem + 3) / 4) * 4;
        }

        int offset = getMemory(totalMem);

        Variable newVar(offset, type, false, false);
        newVar.is_array = true;
        newVar.array_size = arraySize;
        scopes.back()[id] = newVar;
        storeArraySize(id, arraySize);

        return newVar;
    }

    int declareParameter(const std::string& id, TypeSpecifier type, int param_idx, bool isPointer=false) {
        unsigned int t_size = (isPointer) ? POINTER_MEM : getTypeSize(type);
        if (t_size % 4 != 0) {
            t_size = ((t_size + 3) / 4) * 4;
        }

        int offset;

        if (param_idx < 8) {
            offset = getMemory(t_size);
        } else {
            // if no regs left, params are on stack
            offset = TOTAL_STACK_SIZE + (param_idx - 8) * 4;
        }

        Variable param(offset, type, true, isPointer);
        param.is_stack_param = (param_idx >= 8);
        scopes.back()[id] = param;

        // Save parameter for function scope
        if (!parameters_stack.empty()) {
            parameters_stack.back().push_back(param);
        }

        return offset;
    }

    void declareUnnamedParameter(TypeSpecifier type) {
        unsigned int t_size = getTypeSize(type);
        if (t_size % 4 != 0) {
            t_size = ((t_size + 3) / 4) *4;
        }
        getMemory(t_size);
    }

    std::optional<Variable> findVariable(const std::string& id) const {
        if (isGlobal(id)) {
            // find variable in  global scope first
            auto var_it = scopes.front().find(id);
            if (var_it != scopes.front().end()) {
                return var_it->second;
            }
        }

        // then search from inner to outermost scope
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto var_it = it->find(id);
            if (var_it != it->end()) {
                return var_it->second;
            }
        }
        return std::nullopt;
    }

    bool variableExists(const std::string& id) const {
        return findVariable(id).has_value();
    }

    TypeSpecifier getType(const std::string& id) const {
        auto var_opt = findVariable(id);
        if (!var_opt) {
            throw std::runtime_error("GetType Undefined variable: " + id);
        }
        return var_opt->type;
    }

    int getTypeSize(TypeSpecifier type) const {
        return TYPE_SIZE.at(type);
    }

    std::string allocateRegister(const std::set<std::string>& exclude = {}, std::ostream* stream = nullptr) {
        static const std::vector<std::string> all_registers = {
            "t0", "t1", "t2", "t3", "t4", "t5", "t6",
            "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
        };

        for (const auto &reg : all_registers) {
            if ((used_registers.find(reg) == used_registers.end()) &&
                (exclude.find(reg) == exclude.end())) {
                used_registers.insert(reg);
                if (stream != nullptr) {
                    *stream << "    mv " << reg << ", zero" << std::endl;
                }
                return reg;
            }
        }
        throw std::runtime_error("No free registers available");
    }

    std::string allocateFloatingRegister(const std::set<std::string>& exclude = {}, std::ostream* stream = nullptr) {
        static const std::vector<std::string> float_registers = {
            "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
            "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7"
        };

        for (const auto &reg : float_registers) {
            if ((used_float_registers.find(reg) == used_float_registers.end()) &&
                (exclude.find(reg) == exclude.end())) {
                used_float_registers.insert(reg);
                if (stream != nullptr) {
                    // Need a temporary integer register to load 0
                    std::string temp_reg = allocateRegister(exclude);
                    *stream << "    mv " << temp_reg << ", zero" << std::endl;
                    *stream << "    fcvt.s.w " << reg << ", " << temp_reg << std::endl;
                    freeRegister(temp_reg);
                }
                return reg;
            }
        }
        throw std::runtime_error("No free floating-point registers available");
    }

    void freeRegister(const std::string &reg) {
        used_registers.erase(reg);
    }

    void freeFloatingRegister(const std::string& reg) {
        used_float_registers.erase(reg);
    }

    void saveRegisters(std::ostream& stream) {
        // calculate total mem needed then adjust stack pointer
        int totalMem = 0;
        for (const auto& reg : used_registers) {
            if (reg[0] == 't' || reg[0] == 'a') {
                totalMem += 4;
            }
        }

        for (const auto& reg : used_float_registers) {
            if (reg.length() > 2 && reg[1] == 'a') {
                auto funcName = getCurrentFunction();
                auto returnType = getFunctionReturnType(funcName);
                if (returnType == TypeSpecifier::DOUBLE) {
                    totalMem += 8;
                } else {
                    totalMem += 4;
                }
            } else {
                // for temp regs
                totalMem += 4;
            }
        }

        // 16 byte align
        if (totalMem % 16 != 0) {
            totalMem = ((totalMem + 15) / 16) * 16;
        }

        // only adjust sp if need register saving
        if (totalMem > 0) {
            stream << "    addi sp, sp, -" << totalMem << std::endl;
            last_stack_adjust = totalMem;

            // Now save registers at known offsets
            int offset = 0;
            for (const auto& reg : used_registers) {
                if (reg[0] == 't' || reg[0] == 'a') {
                    stream << "    sw " << reg << ", " << offset << "(sp)" << std::endl;
                    saved_registers[reg] = offset;
                    offset += 4;
                }
            }
            // save floating point regs too
            for (const auto& reg : used_float_registers) {
                bool isDouble = false;

                // check if double precision
                if (reg.length() > 2) {
                    auto funcName = getCurrentFunction();
                    auto returnType = getFunctionReturnType(funcName);
                    isDouble = (returnType == TypeSpecifier::DOUBLE);
                }

                if (isDouble) {
                    stream << "    fsd " << reg << ", " << offset << "(sp)" << std::endl;
                    saved_float_registers[reg] = offset;
                    offset += 8;
                } else {
                    stream << "    fsw " << reg << ", " << offset << "(sp)" << std::endl;
                    saved_float_registers[reg] = offset;
                    offset += 4;
                }
            }
        } else {
            last_stack_adjust = 0;
        }
    }

    void restoreRegisters(std::ostream& stream) {
        // Restore caller-saved registers after function call
        for (const auto& [reg, offset] : saved_registers) {
            stream << "    lw " << reg << ", " << offset << "(sp)" << std::endl;
        }

        for (const auto& [reg, offset] : saved_float_registers) {
            bool isDouble = false;

            if (reg.length() > 2) {
                auto funcName = getCurrentFunction();
                auto returnType = getFunctionReturnType(funcName);
                isDouble = (returnType == TypeSpecifier::DOUBLE);
            }

            if (isDouble) {
                stream << "    fld " << reg << ", " << offset << "(sp)" << std::endl;
            } else {
                stream << "    flw " << reg << ", " << offset << "(sp)" << std::endl;
            }
        }

        if (last_stack_adjust > 0) {
            stream << "    addi sp, sp, " << last_stack_adjust << std::endl;
        }
        saved_registers.clear();
        saved_float_registers.clear();
    }


    void loadVariable(std::ostream& stream, const std::string& reg, const std::string& id) {
        auto var_opt = findVariable(id);
        if (!var_opt) {
            throw std::runtime_error("Load Undefined variable: " + id);
        }

        const Variable& var = *var_opt;

        // different handling for parameters
        if (var.is_parameter && var.is_stack_param) {
            // load from positive offset relative to s0
            if (var.type == TypeSpecifier::CHAR && !var.is_pointer) {
                stream << "    lbu " << reg << ", " << var.stack_offset << "(sp)" << std::endl;
            } else if (var.type == TypeSpecifier::FLOAT) {
                stream << "    flw " << reg << ", " << var.stack_offset << "(sp)" << std::endl;
            } else if (var.type == TypeSpecifier::DOUBLE) {
                stream << "    fld " << reg << ", " << var.stack_offset << "(sp)" << std::endl;
            } else {
                stream << "    lw " << reg << ", " << var.stack_offset << "(sp)" << std::endl;
            }
            return;
        }

        if (var.type == TypeSpecifier::FLOAT || var.type == TypeSpecifier::DOUBLE) {
            if (var.type == TypeSpecifier::FLOAT) {
                stream << "    flw " << reg << ", " << var.stack_offset << "(s0)" << std::endl;
            } else {
                stream << "    fld " << reg << ", " << var.stack_offset << "(s0)" << std::endl;
            }
        } else {
            // For integer variables, use lw/lb
            if (var.type == TypeSpecifier::CHAR && !var.is_pointer) {
                stream << "    lbu " << reg << ", " << var.stack_offset << "(s0)" << std::endl;
            } else {
                stream << "    lw " << reg << ", " << var.stack_offset << "(s0)" << std::endl;
            }
        }
    }

    void storeVariable(std::ostream& stream, const std::string& reg, const std::string& id) {
        auto var_opt = findVariable(id);
        if (!var_opt) {
            throw std::runtime_error("Store: Undefined variable: " + id);
        }

        const Variable& var = *var_opt;

        if (var.is_parameter && var.is_stack_param) {
            if (var.type == TypeSpecifier::CHAR && !var.is_pointer) {
                stream << "    sb " << reg << ", " << var.stack_offset << "(sp)" << std::endl;
            } else if (var.type == TypeSpecifier::FLOAT || var.type == TypeSpecifier::DOUBLE) {
                stream << "    fsw " << reg << ", " << var.stack_offset << "(sp)" << std::endl;
            } else {
                stream << "    sw " << reg << ", " << var.stack_offset << "(sp)" << std::endl;
            }
            return;
        }

        // Handle different types
        if (var.type == TypeSpecifier::FLOAT || var.type == TypeSpecifier::DOUBLE) {
            // For floating-point variables, use fsw/fsd
            if (var.type == TypeSpecifier::FLOAT) {
                stream << "    fsw " << reg << ", " << var.stack_offset << "(s0)" << std::endl;
            } else {
                stream << "    fsd " << reg << ", " << var.stack_offset << "(s0)" << std::endl;
            }
        } else {
            // For integer variables, use sw/sb
            if (var.type == TypeSpecifier::CHAR && !var.is_pointer) {
                stream << "    sb " << reg << ", " << var.stack_offset << "(s0)" << std::endl;
            } else {
                stream << "    sw " << reg << ", " << var.stack_offset << "(s0)" << std::endl;
            }
        }
    }

    std::string generateUniqueLabel(const std::string& prefix) {
        return prefix + "_" + std::to_string(label_counter++);
    }

    std::string getFloatLabel(float value) {
        auto it = float_labels.find(value);
        if (it != float_labels.end()) {
            return it->second;
        }
        std::string label = ".FLC_" + std::to_string(float_labels.size() + 1);
        float_labels[value] = label;
        floatValues.push_back(value);
        return label;
    }

    std::string getDoubleLabel(double value) {
        auto it = double_labels.find(value);
        if (it != double_labels.end()) {
            return it->second;
        }
        std::string label = ".DLC_" + std::to_string(double_labels.size() + 1);
        double_labels[value] = label;
        doubleValues.push_back(value);
        return label;
    }

    std::string getStringLabel(std::string value) {
        auto it = string_labels.find(value);
        if (it != string_labels.end()) {
            return it->second;
        }
        std::string label = ".SLC_" + std::to_string(string_labels.size() + 1);
        string_labels[value] = label;
        stringValues.push_back(value);
        return label;
    }

    void pushBreakTarget(const std::string& label) {
        break_targets.push_back(label);
    }

    void popBreakTarget() {
        if (!break_targets.empty()) {
            break_targets.pop_back();
        }
    }

    std::string getCurrentBreakTarget() const {
        if (break_targets.empty()) {
            return "";
        }
        return break_targets.back();
    }

    void setCurrentSwitchValue(const std::string& reg) {current_switch_reg = reg;}
    std::string getCurrentSwitchValue() const {return current_switch_reg;}
    void clearCurrentSwitchValue() {current_switch_reg = "";}

    void pushContinueTarget(const std::string& label) {continue_targets.push_back(label);}
    void popContinueTarget() {
        if(!continue_targets.empty()){
            continue_targets.pop_back();
        }
    }
    std::string getContinueTarget() const {
        if (continue_targets.empty()) {
            return "";
        }
        return continue_targets.back();
    }

    void addEnumType(const EnumType& enumType) {
        const std::string& name = enumType.getName();
        if (!name.empty()) {
            enumTypes[name] = enumType;
        }

        for (const auto& [valueName, value] : enumType.getValues()) {
            enumValues[valueName] = std::make_pair(name, value);
        }
    }

    bool isEnumValue(const std::string& name) const {
        return enumValues.find(name) != enumValues.end();
    }

    int getEnumValue(const std::string& name) const {
        auto it = enumValues.find(name);
        if (it != enumValues.end()) {
            return it->second.second;
        }
        throw std::runtime_error("Unknown enum value");
        return 0;
    }

    std::string getEnumTypeName(const std::string& valueName) const {
        auto it = enumValues.find(valueName);
        if (it != enumValues.end()) {
            return it->second.first;
        }
    }

    void setGlobal(const std::string& id) {
        if (!functionExists(id)) {
            global_variables[id] = true;
        }
    }

    bool isGlobal(const std::string& id) const {
        auto it = global_variables.find(id);
        return it != global_variables.end() && it->second;
    }

    void storeArraySize(std::string name, int size){
        arraySize[name] = size;
    }

    int findArraySize(const std::string& name) {
        auto it = arraySize.find(name);
        if (it != arraySize.end()) {
            return it->second;
        }
        else {
            return 1;
        }
    }

};

} // namespace ast
