#pragma once

#include "ast_base.hpp"
#include <string>
#include <cstdint>

namespace ast {

// Integer type (wire, reg, logic)
class IntType : public Type {
public:
    enum class Kind {
        Wire,    // wire
        Reg,     // reg
        Logic    // logic (SystemVerilog)
    };

    IntType(Kind kind, uint32_t width, bool is_signed = false)
        : kind_(kind), width_(width), is_signed_(is_signed) {}

    std::string emit() const override {
        std::string result;
        if (is_signed_) {
            result += "signed ";
        }

        switch (kind_) {
            case Kind::Wire:  result += "wire"; break;
            case Kind::Reg:   result += "reg"; break;
            case Kind::Logic: result += "logic"; break;
        }

        if (width_ > 1) {
            result += " [" + std::to_string(width_ - 1) + ":0]";
        }

        return result;
    }

    uint32_t getWidth() const override { return width_; }
    bool isSigned() const override { return is_signed_; }
    Kind getKind() const { return kind_; }

private:
    Kind kind_;
    uint32_t width_;
    bool is_signed_;
};

// Array type
class ArrayType : public Type {
public:
    ArrayType(Ptr<Type> element_type, uint32_t size)
        : element_type_(std::move(element_type)), size_(size) {}

    std::string emit() const override {
        return element_type_->emit() + " [0:" + std::to_string(size_ - 1) + "]";
    }

    uint32_t getWidth() const override {
        return element_type_->getWidth() * size_;
    }

    bool isSigned() const override {
        return element_type_->isSigned();
    }

    uint32_t getSize() const { return size_; }
    const Type* getElementType() const { return element_type_.get(); }

private:
    Ptr<Type> element_type_;
    uint32_t size_;
};

// Packed array type (e.g., logic [3:0][7:0])
class PackedArrayType : public Type {
public:
    PackedArrayType(Ptr<Type> element_type, uint32_t packed_dim)
        : element_type_(std::move(element_type)), packed_dim_(packed_dim) {}

    std::string emit() const override {
        return element_type_->emit() + " [" + std::to_string(packed_dim_ - 1) + ":0]";
    }

    uint32_t getWidth() const override {
        return element_type_->getWidth() * packed_dim_;
    }

    bool isSigned() const override {
        return element_type_->isSigned();
    }

    uint32_t getPackedDim() const { return packed_dim_; }
    const Type* getElementType() const { return element_type_.get(); }

private:
    Ptr<Type> element_type_;
    uint32_t packed_dim_;
};

// Helper functions for creating common types
inline Ptr<Type> makeWire(uint32_t width = 1, bool is_signed = false) {
    return make<IntType>(IntType::Kind::Wire, width, is_signed);
}

inline Ptr<Type> makeReg(uint32_t width = 1, bool is_signed = false) {
    return make<IntType>(IntType::Kind::Reg, width, is_signed);
}

inline Ptr<Type> makeLogic(uint32_t width = 1, bool is_signed = false) {
    return make<IntType>(IntType::Kind::Logic, width, is_signed);
}

} // namespace ast
