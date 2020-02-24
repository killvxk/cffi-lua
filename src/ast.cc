#include "ast.hh"

namespace ast {

void c_param::do_serialize(std::string &o) const {
    p_type.do_serialize(o);
    if (!this->name.empty()) {
        if (o.back() != '*') {
            o += ' ';
        }
        o += this->name;
    }
}

void c_function::do_serialize_full(std::string &o, bool fptr, int cv) const {
    p_result.do_serialize(o);
    if (o.back() != '*') {
        o += ' ';
    }
    if (fptr) {
        o += "(*";
    } else {
        o += "(";
    }
    if (cv & C_CV_CONST) {
        if (o.back() != '(') {
            o += ' ';
        }
        o += "const";
    }
    if (cv & C_CV_VOLATILE) {
        if (o.back() != '(') {
            o += ' ';
        }
        o += "volatile";
    }
    o += ")(";
    bool first = true;
    for (auto &p: p_params) {
        if (!first) {
            o += ", ";
            first = false;
        }
        p.do_serialize(o);
    }
    o += ')';
}

c_type::c_type(c_function tp, int qual):
    c_object{}, p_type{C_BUILTIN_FPTR | uint32_t(qual)}
{
    new (&p_ptr.fptr) std::unique_ptr<c_function>{
        std::make_unique<c_function>(std::move(tp))
    };
}

c_type::~c_type() {
    if (type() == C_BUILTIN_FPTR) {
        using T = std::unique_ptr<c_function>;
        p_ptr.fptr.~T();
    } else if (type() == C_BUILTIN_PTR) {
        using T = std::unique_ptr<c_type>;
        p_ptr.ptr.~T();
    }
}

c_type::c_type(c_type &&v): p_type{v.p_type} {
    int tp = type();
    if (tp == C_BUILTIN_FPTR) {
        new (&p_ptr.fptr) std::unique_ptr<c_function>{
            std::move(v.p_ptr.fptr)
        };
    } else if (tp == C_BUILTIN_PTR) {
        new (&p_ptr.ptr) std::unique_ptr<c_type>{
            std::move(v.p_ptr.ptr)
        };
    }
}

c_type &c_type::operator=(c_type &&v) {
    if (type() == C_BUILTIN_FPTR) {
        using T = std::unique_ptr<c_function>;
        p_ptr.fptr.~T();
    } else if (type() == C_BUILTIN_PTR) {
        using T = std::unique_ptr<c_type>;
        p_ptr.ptr.~T();
    }

    p_type = v.p_type;
    if (type() == C_BUILTIN_FPTR) {
        new (&p_ptr.fptr) std::unique_ptr<c_function>{
            std::move(v.p_ptr.fptr)
        };
    } else if (type() == C_BUILTIN_PTR) {
        new (&p_ptr.ptr) std::unique_ptr<c_type>{
            std::move(v.p_ptr.ptr)
        };
    }

    return *this;
}

void c_type::do_serialize(std::string &o) const {
    int tcv = cv();
    int ttp = type();
    if (ttp == C_BUILTIN_PTR) {
        p_ptr.ptr->do_serialize(o);
        if (o.back() != '*') {
            o += ' ';
        }
        o += '*';
    } else if (ttp == C_BUILTIN_FPTR) {
        p_ptr.fptr->do_serialize_full(o, true, tcv);
        return;
    } else {
        switch (type()) {
            case C_BUILTIN_CHAR:
            case C_BUILTIN_SHORT:
            case C_BUILTIN_LONG:
            case C_BUILTIN_LLONG:
                if (tcv & C_CV_UNSIGNED) {
                    o += "unsigned ";
                } else if (tcv & C_CV_SIGNED) {
                    o += "signed ";
                }
                break;
            default:
                break;
        }
        o += this->name;
    }
    if (tcv & C_CV_CONST) {
        o += " const";
    }
    if (tcv & C_CV_VOLATILE) {
        o += " volatile";
    }
}

/* lua is not thread safe, so the FFI doesn't need to be either */

/* the list of declarations; actually stored */
static std::vector<std::unique_ptr<c_object>> decl_list;

/* mapping for quick lookups */
static std::unordered_map<std::string, c_object const *> decl_map;

void add_decl(c_object *decl) {
    decl_list.emplace_back(decl);
    auto &d = *decl_list.back();
    decl_map.emplace(d.name, &d);
}

c_object *lookup_decl(std::string const &name) {
    auto it = decl_map.find(name);
    if (it == decl_map.end()) {
        return nullptr;
    }
    return const_cast<c_object *>(it->second);
}

} /* namespace ast */