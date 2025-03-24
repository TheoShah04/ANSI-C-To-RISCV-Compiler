#pragma once
#include <iostream>
#include <memory>
#include <vector>
#include "ast_context.hpp"



namespace ast{

class Visitor;

class Node {
public:
    virtual ~Node() = default;

    virtual void accept(Visitor& visitor) const = 0;

    virtual void Print(std::ostream& stream) const {
        stream << "Node" << std::endl;
    }
};

// shared_ptr is just easier for now to copy around
// probably should change to unique_ptr later
using NodePtr = std::shared_ptr<Node>;

class NodeList : public Node {
private:
    std::vector<NodePtr> nodes;

public:
    NodeList() = default;
    virtual ~NodeList() = default;

    void PushBack(NodePtr node) {
        nodes.push_back(std::move(node));
    }

    void addAllNodes(const NodeList* otherList) {
        if (!otherList) return;
        for (const auto& node : otherList->nodes) {
            nodes.push_back(node);
        }
    }

    const std::vector<NodePtr>& getNodes() const {
        return nodes;
    }

    size_t size() const {
        return nodes.size();
    }

    bool empty() const {
        return nodes.empty();
    }

    // Access nodes by index
    const Node* at(size_t index) const {
        return index < nodes.size() ? nodes[index].get() : nullptr;
    }

    void accept(Visitor& visitor) const override {
        for (const auto& node : nodes){
            node->accept(visitor);
        }
    }

    void Print(std::ostream& stream) const override {
        stream << "NodeList [" << std::endl;
        for (const auto& node : nodes) {
            stream << "  ";
            node->Print(stream);
        }
        stream << "]" << std::endl;
    }
};

} //namespace ast
