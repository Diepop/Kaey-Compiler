// ReSharper disable CppDeclaratorNeverUsed
#include "CreateAst.hpp"

#include "Expressions.hpp"
#include "Statements.hpp"
#include "Types.hpp"

#include "Kaey/Parser/KaeyParser.hpp"

using namespace Kaey::Lexer;

namespace Kaey::Ast
{
    namespace
    {
        struct ConstexprEvaluator : Expression::Visitor<Expression*>
        {

            Expression* Visit(BooleanConstant* arg)
            {
                return arg;
            }

            Expression* Visit(CharacterConstant* arg)
            {
                return arg;
            }

            Expression* Visit(FloatConstant* arg)
            {
                return arg;
            }

            Expression* Visit(IntegerConstant* arg)
            {
                return arg;
            }

            Expression* Visit(VoidConstant* arg)
            {
                return arg;
            }

            Expression* Visit(TupleExpression* arg)
            {
                return ctx->CreateObject<TupleExpression>(arg->TypeOfTuple(), VisitRange(arg->Values(), this));
            }

            Expression* Visit(BinaryExpression* arg)
            {
                auto left = Dispatch(arg->LeftOperand(), this);
                auto right = Dispatch(arg->RightOperand(), this);
                if (!left || !right)
                    return nullptr;
                return VariadicVisit<Expression*>({ left, right },
                    [&](IntegerConstant* l, IntegerConstant* r)
                    {
                        auto lType = l->TypeOfInteger();
                        auto rType = r->TypeOfInteger();
                        assert(lType->IsSigned() == rType->IsSigned());
                        auto type = lType->Bits() > rType->Bits() ? lType : rType;
                        auto bType = ctx->GetBooleanType();
                        return Dispatch<Expression*>(arg->Operator(),
                            [=](Plus*) { return type->CreateConstant(l->Value() + r->Value()); },
                            [=](Minus*) { return type->CreateConstant(l->Value() - r->Value()); },
                            [=](Star*) { return type->CreateConstant(l->Value() * r->Value()); },
                            [=](Slash*) { return type->CreateConstant(l->Value() / r->Value()); },
                            [=](Modulo*) { return type->CreateConstant(l->Value() % r->Value()); },

                            [=](Caret*) { return type->CreateConstant(l->Value() ^ r->Value()); },
                            [=](Pipe*) { return type->CreateConstant(l->Value() | r->Value()); },
                            [=](Ampersand*) { return type->CreateConstant(l->Value() & r->Value()); },

                            [=](LeftShift*) { return type->CreateConstant(l->Value() << r->Value()); },
                            [=](RightShift*) { return type->CreateConstant(l->Value() >> r->Value()); },

                            [=](Equal*) { return bType->CreateConstant(l->Value() == r->Value()); },
                            [=](NotEqual*) { return bType->CreateConstant(l->Value() != r->Value()); },
                            [=](Less*) { return bType->CreateConstant(l->Value() < r->Value()); },
                            [=](LessEqual*) { return bType->CreateConstant(l->Value() <= r->Value()); },
                            [=](Greater*) { return bType->CreateConstant(l->Value() > r->Value()); },
                            [=](GreaterEqual*) { return bType->CreateConstant(l->Value() >= r->Value()); },

                            [=](Token*) { return nullptr; }
                        );
                    },
                    [&](BooleanConstant* l, BooleanConstant* r)
                    {
                        auto type = ctx->GetBooleanType();
                        return Dispatch<Expression*>(arg->Operator(),
                            [=](Equal*) { return type->CreateConstant(l->Value() == r->Value()); },
                            [=](NotEqual*) { return type->CreateConstant(l->Value() != r->Value()); },

                            [=](And*) { return type->CreateConstant(l->Value() && r->Value()); },
                            [=](Or*) { return type->CreateConstant(l->Value() || r->Value()); },

                            [=](Token*) { return nullptr; }
                        );
                    },
                    [&](FloatConstant* l, FloatConstant* r)
                    {
                        auto type = l->TypeOfFloat();
                        auto bType = ctx->GetBooleanType();
                        return Dispatch<Expression*>(arg->Operator(),
                            [=](Plus*) { return type->CreateConstant(l->Value() + r->Value()); },
                            [=](Minus*) { return type->CreateConstant(l->Value() - r->Value()); },
                            [=](Star*) { return type->CreateConstant(l->Value() * r->Value()); },
                            [=](Slash*) { return type->CreateConstant(l->Value() / r->Value()); },

                            [=](Equal*) { return bType->CreateConstant(l->Value() == r->Value()); },
                            [=](NotEqual*) { return bType->CreateConstant(l->Value() != r->Value()); },
                            [=](Less*) { return bType->CreateConstant(l->Value() < r->Value()); },
                            [=](LessEqual*) { return bType->CreateConstant(l->Value() <= r->Value()); },
                            [=](Greater*) { return bType->CreateConstant(l->Value() > r->Value()); },
                            [=](GreaterEqual*) { return bType->CreateConstant(l->Value() >= r->Value()); },

                            [=](Token*) { return nullptr; }
                        );
                    },
                    [&](Expression*, Expression*) { return nullptr; }
                );
            }

            Expression* Visit(UnaryExpression* arg)
            {
                auto operand = Dispatch(arg->Operand(), this);
                if (!operand)
                    return nullptr;
                return Dispatch<Expression*>(operand,
                    [&](IntegerConstant* v)
                    {
                        auto type = v->TypeOfInteger();
                        return Dispatch<Expression*>(arg->Operator(),
                            [=](Plus*) { return type->CreateConstant(+v->Value()); },
                            [=](Minus*) { return type->CreateConstant(-v->Value()); },
                            [=](Tilde*) { return type->CreateConstant(~v->Value()); },
                            [=](Token*) { return nullptr; }
                        );
                    },
                    [&](BooleanConstant* v)
                    {
                        auto type = ctx->GetBooleanType();
                        return Dispatch<Expression*>(arg->Operator(),
                            [=](Exclamation*) { return type->CreateConstant(!v->Value()); },
                            [=](Token*) { return nullptr; }
                        );
                    },
                    [&](FloatConstant* v)
                    {
                        auto type = v->TypeOfFloat();
                        return Dispatch<Expression*>(arg->Operator(),
                            [=](Plus*) { return type->CreateConstant(+v->Value()); },
                            [=](Minus*) { return type->CreateConstant(-v->Value()); },
                            [=](Token*) { return nullptr; }
                        );
                    },
                    [&](Expression*, Expression*) { return nullptr; }
                );
            }

            explicit ConstexprEvaluator(Context* ctx) : ctx(ctx) {  }

        private:
            Context* ctx;

        };

        struct ExpressionVisitor;

        struct TypeVisitor : Parser::BaseType::Visitor<Type*>
        {

            Type* Visit(Parser::DecoratedType* arg)
            {
                auto type = Dispatch(arg->UnderlyingType, this);
                switch (arg->Decorator->KindId())
                {
                case Star::Id: return type->PointerTo();
                case Ampersand::Id: return type->ReferenceTo();
                case Question::Id: return type->OptionalTo();
                default: throw std::runtime_error("Invalid Decorator!");
                }
            }

            Type* Visit(Parser::TupleType* arg)
            {
                return ctx->GetTupleType(VisitRange(arg->Types, this));
            }

            Type* Visit(Parser::IdentifierType* arg)
            {
                auto type = ctx->FindType(arg->Identifier);
                assert(type);
                return type;
            }

            Type* Visit(Parser::ArrayType* arg);

            Type* Visit(Parser::VariantType* arg)
            {
                return ctx->GetVariantType(VisitRange(arg->UnderlyingTypes, this));
            }

            TypeVisitor(Context* ctx, ExpressionVisitor* expressionVisitor) : ctx(ctx), constexprEvaluator(ctx), expressionVisitor(expressionVisitor) {  }

        private:
            Context* ctx;
            ConstexprEvaluator constexprEvaluator;
            ExpressionVisitor* expressionVisitor;
        };

        struct ExpressionVisitor : Parser::Expression::Visitor<Expression*>
        {

            Expression* Visit(Parser::IntegerExpression* arg)
            {
                return ctx->GetIntegerType(arg->Bits, arg->IsSigned)->CreateConstant(arg->Value);
            }

            Expression* Visit(Parser::BooleanExpression* arg)
            {
                return ctx->GetBooleanType()->CreateConstant(arg->Value);
            }

            Expression* Visit(Parser::FloatingExpression* arg)
            {
                return ctx->GetFloatType()->CreateConstant(arg->Value);
            }

            Expression* Visit(Parser::StringLiteralExpression* arg)
            {
                auto ct = ctx->GetCharacterType();
                auto at = ctx->GetArrayType(ct, (int64_t)arg->Value.size());
                auto values = arg->Value | vs::transform([=](char d) -> Expression* { return ct->CreateConstant(d); }) | to_vector;
                return at->CreateExpression(move(values));
            }

            Expression* Visit(Parser::TupleExpression* arg)
            {
                auto exprs = VisitRange(arg->Expressions, this);
                auto types = exprs | vs::transform([](auto expr) { return expr->Type()->RemoveReference(); }) | to_vector;
                return ctx->GetTupleType(move(types))->CreateExpression(move(exprs));
            }

            Expression* Visit(Parser::ParenthisedExpression* arg)
            {
                return Dispatch(arg->UnderlyingExpression, this);
            }

            Expression* Visit(Parser::BinaryOperatorExpression* arg)
            {
                auto left = Dispatch(arg->Left, this);
                auto right = Dispatch(arg->Right, this);
                auto fn = ctx->FindFunction("operator{}"_f(arg->Operation->Text()));
                auto overs = fn->FindCallable({ left->Type(), right->Type() });
                assert(!overs.empty());
                auto type = overs[0]->ReturnType();
                return ctx->CreateObject<BinaryExpression>(type, left, arg->Operation, right);
            }

            Expression* Visit(Parser::UnaryOperatorExpression* arg)
            {
                auto v = Dispatch(arg->Operand, this);
                assert(v);
                auto ty = v->Type()->RemoveReference();
                auto f = ty->FindMethod("operator{}"_f(arg->Operation->Text()));
                assert(f);
                auto op = f->FindOverload({ v->Type() });
                assert(op);
                return ctx->CreateObject<UnaryExpression>(op->ReturnType(), arg->Operation, v);
            }

            Expression* Visit(Parser::IdentifierExpression* arg)
            {
                if (arg->Identifier == "ExternC")
                    return ctx->CreateObject<ExternCAttribute>();
                if (auto f = ctx->FindFunction(arg->Identifier))
                    return f;
                if (auto v = ctx->FindVariable(arg->Identifier))
                    return v->Reference();
                return nullptr;
            }

            Expression* Visit(Parser::CallExpression* arg)
            {
                auto callee = Dispatch(arg->Callee, this);
                if (!callee)
                if (auto id = arg->Callee->As<Parser::IdentifierExpression>())
                {
                    auto ty = ctx->FindType(id->Identifier);
                    assert(ty);
                    return ctx->CreateObject<ConstructExpression>(ty, VisitRange(arg->Arguments, this));
                }
                assert(callee);
                auto args = VisitRange(arg->Arguments, this);
                auto type = Dispatch<Type*>(callee,
                    [&](Function* fn)
                    {
                        auto types = args | type_of_range | to_vector;
                        auto overloads = fn->FindCallable(types);
                        assert(overloads.size() == 1);
                        return overloads.front()->ReturnType();
                    }
                );
                return ctx->CreateObject<CallExpression>(type, callee, move(args));
            }

            Expression* Visit(Parser::TernaryExpression* arg)
            {
                auto cond  = Dispatch(arg->Condition, this);
                auto texpr = Dispatch(arg->TrueExpression, this);
                auto fexpr = Dispatch(arg->FalseExpression, this);
                return ctx->CreateObject<TernaryExpression>(texpr->Type(), cond, texpr, fexpr);
            }

            Expression* Visit(Parser::NewExpression* arg)
            {
                auto type = Dispatch(arg->Type, typeVisitor)->PointerTo();
                auto args = VisitRange(arg->Arguments, this);
                return ctx->CreateObject<NewExpression>(type, move(args));
            }

            Expression* Visit(Parser::NamedOperatorExpression* arg)
            {
                switch (arg->Token->KindId())
                {
                case Delete::Id: return ctx->CreateObject<DeleteExpression>(Dispatch(arg->Argument, this));
                case Return::Id: return ctx->CreateObject<ReturnExpression>(Dispatch(arg->Argument, this));
                default: throw std::runtime_error("Not Implemented!");
                }
            }

            Expression* Visit(Parser::NamedTypeOperatorExpression* arg)
            {
                auto isize = ctx->GetIntegerType(sizeof(size_t) * 8, true);
                switch (arg->Token->KindId())
                {
                case SizeOf::Id: return ctx->CreateObject<SizeOfExpression>(isize, Dispatch(arg->Type, typeVisitor));
                case AlignOf::Id: return ctx->CreateObject<AlignOfExpression>(isize, Dispatch(arg->Type, typeVisitor));
                default: throw std::runtime_error("Not Implemented!");
                }
            }

            Expression* Visit(Parser::SubscriptOperator* arg)
            {
                auto callee = Dispatch(arg->Callee, this);
                assert(callee);
                auto args = VisitRange(arg->Arguments, this);
                auto type = callee->Type()->RemoveReference();
                auto f = type->FindMethod(SUBSCRIPT_OPERATOR);
                assert(f);
                auto types = args | type_of_range | to_vector;
                types.insert(types.begin(), callee->Type());
                auto overs = f->FindCallable(types);
                assert(!overs.empty());
                type = overs[0]->Type()->ReturnType();
                return ctx->CreateObject<SubscriptOperatorExpression>(type, callee, move(args));
            }

            Expression* Visit(Parser::MemberAccessExpression* arg)
            {
                auto owner = Dispatch(arg->Owner, this);
                auto ty = owner->Type()->RemoveReference();
                auto field = ty->FindField(arg->Member->Identifier);
                if (!field && ty->Is<PointerType>())
                    field = ty->RemovePointer()->FindField(arg->Member->Identifier);
                return ctx->CreateObject<MemberAccessExpression>(owner, field);
            }

            ExpressionVisitor(Context* ctx) : ctx(ctx), typeVisitor(ctx, this) {  }

        private:
            Context* ctx;
            TypeVisitor typeVisitor;

        };

        Type* TypeVisitor::Visit(Parser::ArrayType* arg)
        {
            auto uType = Dispatch(arg->UnderlyingType, this);
            auto vs = VisitRange(arg->LengthExpressions, expressionVisitor);
            auto values = VisitRange(vs, constexprEvaluator) | vs::transform([](Expression* expr) { return expr->As<IntegerConstant>(); }) | to_vector;
            assert(!rn::contains(values, nullptr));
            //TODO implement multidimensional ArrayType.
            return ctx->GetArrayType(uType, (int64_t)values[0]->Value());
        }

        struct ClassVisitor final : ExpressionVisitor, TypeVisitor
        {
            using ExpressionVisitor::Visit;
            using TypeVisitor::Visit;

            void Visit(Parser::FunctionDeclaration* arg)
            {

            }

            void Visit(Parser::ClassDeclaration* arg)
            {
                
            }

            ClassVisitor(ClassDeclaration* decl) : ExpressionVisitor(decl), TypeVisitor(decl, this), decl(decl)
            {
                
            }

        private:
            ClassDeclaration* decl;

        };

        struct StatementVisitor final : Parser::Statement::Visitor<Statement*>, ExpressionVisitor, TypeVisitor
        {
            using ExpressionVisitor::Visit;
            using TypeVisitor::Visit;

            Statement* Visit(Parser::VariableDeclaration* arg)
            {
                auto type = Dispatch(arg->Type, this);
                auto expr = Dispatch(arg->InitializingExpression, this);
                assert(type || expr);
                if (!type)
                    type = expr->Type()->RemoveReference();
                return ctx->CreateChild<VariableDeclaration>(arg->Name, type, expr);
            }

            Statement* Visit(Parser::TupleVariableDeclaration* arg)
            {
                auto ty = Dispatch(arg->Type, this);
                assert(!ty || ty->Is<TupleType>());
                auto type = ty ? ty->As<TupleType>() : nullptr;
                auto expr = Dispatch(arg->InitializingExpression, this);
                assert(type || expr);
                if (!type)
                    type = expr->Type()->RemoveReference()->As<TupleType>();
                //TODO Fix!
                return nullptr;
                //auto vars = type->ElementTypes() | vs::transform([](Type* ty) { return fn->CreateObject<MemberAccessExpression>(expr, type->Fields()[i]); })
                //vector<VariableDeclaration*> vars;
                //for (size_t i = 0; i < types.size(); ++i)
                //{
                //    auto init = fn->CreateObject<MemberAccessExpression>(expr, type->Fields()[i]);
                //    vars.emplace_back(sys->CreateObject<VariableDeclaration>(arg->Names[i], types[i], init));
                //}
                //auto var = sys->CreateObject<TupleVariableDeclaration>(move(vars), type, expr);
                //for (auto v : var->Variables())
                //    varMap.emplace(v->Name(), v->Reference());
                //return var;
            }

            Statement* Visit(Parser::BlockStatement* arg)
            {
                auto bs = ctx->CreateChild<BlockStatement>();
                auto vis = StatementVisitor(bs);
                for (auto s : arg->Statements)
                    bs->Statements.emplace_back(Dispatch(s, vis));
                return bs;
            }

            Statement* Visit(Parser::IfStatement* arg)
            {
                auto ifs = ctx->CreateChild<IfStatement>();
                auto vis = StatementVisitor(ifs);
                ifs->Condition = Dispatch(arg->Condition, vis);
                ifs->TrueStatement = Dispatch(arg->TrueStatement, vis);
                ifs->FalseStatement = Dispatch(arg->FalseStatement, vis);
                return ifs;
            }

            Statement* Visit(Parser::ExpressionStatement* arg)
            {
                auto es = ctx->CreateChild<ExpressionStatement>();
                es->UnderlyingExpression = Dispatch(arg->UnderlyingExpression, StatementVisitor(es));
                return es;
            }

            Statement* Visit(Parser::WhileStatement* arg)
            {
                auto ws = ctx->CreateChild<WhileStatement>();
                auto vis = StatementVisitor(ws);
                ws->Condition = Dispatch(arg->Condition, vis);
                ws->LoopStatement = Dispatch(arg->LoopStatement, vis);
                return ws;
            }

            Statement* Visit(Parser::ForStatement* arg)
            {
                auto fs = ctx->CreateChild<ForStatement>();
                auto vis = StatementVisitor(fs);
                fs->StartStatement  = Dispatch(arg->StartStatement, vis);
                fs->Condition       = Dispatch(arg->Condition, vis);
                fs->Increment       = Dispatch(arg->Increment, vis);
                fs->LoopStatement   = Dispatch(arg->LoopStatement, vis);
                return fs;
            }

            Statement* Visit(Parser::SwitchStatement* arg)
            {
                auto ss = ctx->CreateChild<SwitchStatement>();
                auto vis = StatementVisitor(ss);
                ss->Condition = Dispatch(arg->Condition, vis);
                ss->Cases = arg->Cases | vs::transform([&](auto& p) { return std::make_pair(Dispatch(p.first, vis), Dispatch(p.second, vis)); }) | to_vector;
                return ss;
            }

            StatementVisitor(Context* ctx) : ExpressionVisitor(ctx), TypeVisitor(ctx, this), ctx(ctx)
            {

            }

        private:
            Context* ctx;

        };

        struct ModuleVisitor final : ExpressionVisitor, TypeVisitor, Parser::Statement::Visitor<void>
        {
            using ExpressionVisitor::Visit;
            using TypeVisitor::Visit;

            void Visit(Parser::FunctionDeclaration* arg)
            {
                auto fn = fnMap.at(arg);
                for (auto i : irange((int)arg->Parameters.size())) if (auto ie = arg->Parameters[i]->InitializingExpression)
                    fn->Parameter(i)->AddInitializingExpression(Dispatch(ie, this));
                auto vis = StatementVisitor(fn);
                if (auto e = arg->LambdaReturn)
                {
                    auto expr = Dispatch(e, vis);
                    auto st = fn->CreateChild<ExpressionStatement>();
                    st->UnderlyingExpression = fn->CreateObject<ReturnExpression>(expr);
                    fn->AddStatement(st);
                }
                else for (auto statement : arg->Statements)
                    fn->AddStatement(Dispatch(statement, vis));
            }

            void Visit(Parser::ClassDeclaration* arg)
            {
                auto c = mod->FindType(arg->Name)->As<ClassType>();
                ClassVisitor(c->Declaration()).Visit(arg);
            }

            void Visit(Parser::VariableDeclaration* arg)
            {
                auto v = mod->FindVariable(arg->Name);
                v->AddInitializingExpression(Dispatch(arg->InitializingExpression, this));
            }

            void Visit(Parser::Module* parseTree)
            {
                //Declaring classes.
                VisitRange(parseTree->Statements,
                    [&](Parser::ClassDeclaration* decl)
                    {
                        mod->DeclareClass(decl->Name);
                    }
                );

                auto expressionVisitor = ExpressionVisitor(mod);
                auto typeVisitor = TypeVisitor(mod, &expressionVisitor);

                auto parseParams = vs::transform([&](Parser::VariableDeclaration* decl)
                {
                    auto ty = Dispatch(decl->Type, typeVisitor);
                    if (decl->TypeMod && decl->TypeMod->IsOr<In, Out, Ref>())
                        ty = ty->ReferenceTo();
                    return std::make_pair(decl->Name, ty);
                });

                //Declaring global variables, functions and class fields and methods.
                VisitRange(parseTree->Statements,
                    [&](Parser::FunctionDeclaration* fn)
                    {
                        auto params = fn->Parameters | parseParams | to_vector;
                        auto f = mod->DeclareFunction(fn->Name, move(params), Dispatch(fn->ReturnType, typeVisitor), fn->IsVariadic);
                        for (auto att : fn->Attributes)
                        {
                            auto exprs = VisitRange(att->Expressions, expressionVisitor);
                            f->AddAttribute(move(exprs));
                        }
                        fnMap.emplace(fn, f);
                    },
                    [&](Parser::VariableDeclaration* var)
                    {
                        mod->DeclareVariable(var->Name, Dispatch(var->Type, typeVisitor));
                    },
                    [&](Parser::ClassDeclaration* parsed)
                    {
                        auto type = mod->FindType(parsed->Name);
                        if (auto dc = parsed->DefaultConstructor)
                            for (auto param : dc->Parameters) if (param->DeclarationToken->Is<Var>())
                                type->DeclareField(param->Name, Dispatch(param->Type, typeVisitor));
                        VisitRange(parsed->Statements,
                            [&](Parser::VariableDeclaration* field)
                            {
                                type->DeclareField(field->Name, Dispatch(field->Type, typeVisitor));
                            },
                            [&](Parser::FunctionDeclaration* fn)
                            {
                                auto params = fn->Parameters | parseParams | to_vector;
                                type->DeclareMethod(fn->Name, move(params), Dispatch(fn->ReturnType, typeVisitor));
                            }
                        );

                    }
                );

                return VisitRange(parseTree->Statements, this);
            }

            ModuleVisitor(Module* mod) : ExpressionVisitor(mod), TypeVisitor(mod, this), mod(mod) {  }

        private:
            Module* mod;
            unordered_map<Parser::FunctionDeclaration*, FunctionDeclaration*> fnMap;
        };

    }

    Module* CreateAst(SystemModule* sys, Parser::Module* parseTree)
    {
        auto mod = sys->CreateObject<Module>(parseTree->Name);
        ModuleVisitor(mod).Visit(parseTree);
        return mod;
    }

}
