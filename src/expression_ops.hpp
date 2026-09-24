/*
 *
 *    expression_ops.hpp
 *    Version 1.00 (C++17)  
 * 
 *    Copyright 2026 Steven Janes (www.perfectconsulting.co.uk)
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once
#include <cmath>
#include <iostream>
#include "expression.hpp"
#include "statement.hpp"
#include "value.hpp"
#include "error.hpp"

namespace Basic::Ops {

// Maths Operations
// ----------------------------------------------------------------------------------------------------------

inline ExpressionPtr MakeAdd(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return args[0].AddOp(args[1], env);
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " + " + ops[1]->List();
        });
}

inline ExpressionPtr MakeSub(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return args[0].SubOp(args[1], env);
        },
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " - " + ops[1]->List();
        });
}

inline ExpressionPtr MakeMul(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return args[0].MulOp(args[1], env);
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " * " + ops[1]->List();
        });
}

inline ExpressionPtr MakeDiv(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return args[0].DivOp(args[1], env);
        },
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " / " + ops[1]->List();
        });
}

inline ExpressionPtr MakePow(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return args[0].PowOp(args[1], env);
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " ^ " + ops[1]->List();
        });
}

inline ExpressionPtr MakeEqual(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].CompareOp(args[1], env) == 0);
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " = " + ops[1]->List();
        });
};

inline ExpressionPtr MakeNotEqual(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].CompareOp(args[1], env) != 0);
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " <> " + ops[1]->List();
        });
}

inline ExpressionPtr MakeLessThan(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].CompareOp(args[1], env) < 0);
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " < " + ops[1]->List();
        });
}

inline ExpressionPtr MakeLessEqual(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].CompareOp(args[1], env) <= 0);
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " <= " + ops[1]->List();
        });
}

inline ExpressionPtr MakeGreaterThan(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].CompareOp(args[1], env) > 0);
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " > " + ops[1]->List();
        });           
}

inline ExpressionPtr MakeGreaterEqual(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].CompareOp(args[1], env) >= 0);
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return ops[0]->List() + " >= " + ops[1]->List();
        });             
}

inline ExpressionPtr MakeAbs(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].AbsOp(env));
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return "ABS(" + ops[0]->List() + ")";  
        });                      
}

inline ExpressionPtr MakeAtn(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].AtnOp(env));
        },
        [](const std::vector<ExpressionPtr>& ops) {
                return "ATN(" + ops[0]->List() + ")";  
        });
}

inline ExpressionPtr MakeCos(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].CosOp(env));
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return "COS(" + ops[0]->List() + ")";  
        });
}

inline ExpressionPtr MakeSin(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].SinOp(env));
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return "SIN(" + ops[0]->List() + ")";  
        });
}

inline ExpressionPtr MakeTan(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].TanOp(env));
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return "TAN(" + ops[0]->List() + ")";  
        });
}

inline ExpressionPtr MakeExp(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].ExpOp(env));
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return "EXP(" + ops[0]->List() + ")";  
        });
}

inline ExpressionPtr MakeSqr(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].SqrOp(env));
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return "SQR(" + ops[0]->List() + ")";  
        });
}

inline ExpressionPtr MakeInt(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].IntOp(env));
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return "INT(" + ops[0]->List() + ")";  
        });
}

inline ExpressionPtr MakeSgn(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].SgnOp(env));
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return "SGN(" + ops[0]->List() + ")";  
        });
}

inline ExpressionPtr MakeLog(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].LogOp(env));
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return "LOG(" + ops[0]->List() + ")";  
        });
}

inline ExpressionPtr MakeRnd(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(args[0].RndOp(env));
        }, 
        [](const std::vector<ExpressionPtr>& ops) {
                return "RND(" + ops[0]->List() + ")";  
        });
}

// String Operations
// ----------------------------------------------------------------------------------------------------------

inline ExpressionPtr MakeLen(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            return Value(static_cast<Integer>(args[0].TryGetString(env, "LEN").size()));
        }, [](const std::vector<ExpressionPtr>& ops) { 
                return "LEN(" + ops[0]->List() + ")";
        });
}

inline ExpressionPtr MakeAsc(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            const std::string& s = args[0].TryGetString(env, "ASC");
            if (s.empty()) throw RuntimeError(env, "ASC: empty string");
            return Value(static_cast<Integer>(static_cast<unsigned char>(s[0])));
        }, [](const std::vector<ExpressionPtr>& ops) {
            return "ASC(" + ops[0]->List() + ")";
        });
}

inline ExpressionPtr MakeChr(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            Integer code = static_cast<Integer>(args[0].TryGetInteger(env, "CHR$"));
            return Value(std::string(1, static_cast<char>(code)));
        }, [](const std::vector<ExpressionPtr>& ops) {
            return "CHR$(" + ops[0]->List() + ")";
        });
}

inline ExpressionPtr MakeStr(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            const Value& v = args[0];
            if (!v.IsNumber())
                throw RuntimeError(env, "STR$: expected numeric argument");
            return Value(v.PPrint());
        }, [](const std::vector<ExpressionPtr>& ops) {
            return "STR$(" + ops[0]->List() + ")";
        });
}

inline ExpressionPtr MakeVal(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            const std::string& s = args[0].TryGetString(env, "VAL");

            Basic::Value val(s);
            val.NormaliseType();

            return val;
        }, [](const std::vector<ExpressionPtr>& ops) {
            return "VAL(" + ops[0]->List() + ")";
        });
}

inline ExpressionPtr MakeSpc(ExpressionPtr arg) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(arg)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            long n = static_cast<long>(args[0].TryGetInteger(env, "SPC"));
            return Value(std::string(std::max(0L, n), ' '));
        }, [](const std::vector<ExpressionPtr>& ops) {
            return "SPC(" + ops[0]->List() + ")";
        });
}

inline ExpressionPtr MakeLeft(ExpressionPtr s, ExpressionPtr n) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(s), std::move(n)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            std::string s = args[0].TryGetString(env, "LEFT$");
            long n = args[1].TryGetInteger(env, "LEFT$");
            return Value(s.substr(0, std::min((size_t)std::max(0L, n), s.size())));
        }, [](const std::vector<ExpressionPtr>& ops) {
            return "LEFT$(" + ops[0]->List() + ", " + ops[1]->List() + ")";
        });
}

inline ExpressionPtr MakeRight(ExpressionPtr s, ExpressionPtr n) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(s), std::move(n)},
        [](const std::vector<Value>& args, Environment& env) -> Value {
            std::string s = args[0].TryGetString(env, "RIGHT$");
            long n = args[1].TryGetInteger(env, "RIGHT$");
            long start = (long)s.size() - n;
            if (start < 0) start = 0;
            return Value(s.substr(start));
        }, [](const std::vector<ExpressionPtr>& ops) {
            return "RIGHT$(" + ops[0]->List() + ", " + ops[1]->List() + ")";
        });
}

inline ExpressionPtr MakeMid(ExpressionPtr s, ExpressionPtr start, ExpressionPtr len = nullptr) {
    bool hasLen = (len != nullptr);
    std::vector<ExpressionPtr> operands{std::move(s), std::move(start)};
    if (hasLen) operands.push_back(std::move(len));
    return std::make_shared<LambdaExpression>(std::move(operands),
        [hasLen](const std::vector<Value>& args, Environment& env) -> Value {
            const Value& sv = args[0];
            if (!sv.IsString())
                throw RuntimeError(env, "MID$: expected string argument");

            const std::string s = sv.TryGetString(env, "MID$");

            const Value& startv = args[1];
            if (!startv.IsNumber())
                throw RuntimeError(env, "MID$: expected numeric argument");

            Integer start = startv.TryGetInteger(env, "MID$");

            Integer len = 0;
            if (hasLen) {
                const Value& lenv = args[2];
                if (!lenv.IsNumber())
                    throw RuntimeError(env, "MID$: expected numeric argument");

                len = lenv.TryGetInteger(env, "MID$");
            }
            else {
                len = s.size() - start;
            }

            if (start <= 0) start = 1;
            if (start > (long)s.size()) return Value(std::string(""));
            if (hasLen) {
                return Value(s.substr(start - 1, len));
            }
            return Value(s.substr(start - 1));
        }, [hasLen](const std::vector<ExpressionPtr>& ops) {
            std::string s = "MID$(" + ops[0]->List() + ", " + ops[1]->List();
            if (hasLen) s += ", " + ops[2]->List();
            return s + ")";
        });
}

// Logical Operations
// ----------------------------------------------------------------------------------------------------------

inline ExpressionPtr MakeAnd(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment&) -> Value {
            return Value(static_cast<Integer>(args[0].GetTruthy() && args[1].GetTruthy() ? 1 : 0));
        }, [](const std::vector<ExpressionPtr>& ops) {
            return ops[0]->List() + " AND " + ops[1]->List();
        });
}

inline ExpressionPtr MakeOr(ExpressionPtr l, ExpressionPtr r) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(l), std::move(r)},
        [](const std::vector<Value>& args, Environment&) -> Value {
            return Value(static_cast<Integer>(args[0].GetTruthy() || args[1].GetTruthy() ? 1 : 0));
        }, [](const std::vector<ExpressionPtr>& ops) {
            return ops[0]->List() + " OR " + ops[1]->List();
        });
}

inline ExpressionPtr MakeNot(ExpressionPtr operand) {
    return std::make_shared<LambdaExpression>(std::vector<ExpressionPtr>{std::move(operand)},
        [](const std::vector<Value>& args, Environment&) -> Value {
            return Value(static_cast<Integer>(args[0].GetTruthy() ? 0 : 1));
        }, [](const std::vector<ExpressionPtr>& ops) {
            return "NOT " + ops[0]->List();
        });
}


} // namespace Basic::Ops
