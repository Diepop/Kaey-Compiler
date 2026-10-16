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

    }

    Type* TypeVisitor::Visit(Parser::DecoratedType* arg)
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

    Type* TypeVisitor::Visit(Parser::TupleType* arg)
    {
        return ctx->GetTupleType(VisitRange(arg->Types, this));
    }

    Type* TypeVisitor::Visit(Parser::IdentifierType* arg)
    {
        auto type = ctx->FindType(arg->Identifier);
        assert(type);
        return type;
    }

    Type* TypeVisitor::Visit(Parser::ArrayType* arg)
    {
        auto uType = Dispatch(arg->UnderlyingType, this);
        auto vs = VisitRange(arg->LengthExpressions, expressionVisitor);
        auto values = VisitRange(vs, constexprEvaluator) | vs::transform([](Expression* expr) { return expr->As<IntegerConstant>(); }) | to_vector;
        assert(!rn::contains(values, nullptr));
        //TODO implement multidimensional ArrayType.
        return ctx->GetArrayType(uType, (int64_t)values[0]->Value());
    }

    Type* TypeVisitor::Visit(Parser::VariantType* arg)
    {
        return ctx->GetVariantType(VisitRange(arg->UnderlyingTypes, this));
    }

    TypeVisitor::TypeVisitor(Context* ctx, ExpressionVisitor* expressionVisitor) :
        ctx(ctx),
        constexprEvaluator(ctx),
        expressionVisitor(expressionVisitor)
    {
        
    }

    Expression* ExpressionVisitor::Visit(Parser::IntegerExpression* arg)
    {
        return ctx->GetIntegerType(arg->Bits, arg->IsSigned)->CreateConstant(arg->Value);
    }

    Expression* ExpressionVisitor::Visit(Parser::BooleanExpression* arg)
    {
        return ctx->GetBooleanType()->CreateConstant(arg->Value);
    }

    Expression* ExpressionVisitor::Visit(Parser::FloatingExpression* arg)
    {
        return ctx->GetFloatType(32)->CreateConstant(arg->Value);
    }

    Expression* ExpressionVisitor::Visit(Parser::StringLiteralExpression* arg)
    {
        auto ct = ctx->GetCharacterType();
        auto at = ctx->GetArrayType(ct, (int64_t)arg->Value.size());
        auto values = arg->Value | vs::transform([=](char d) -> Expression* { return ct->CreateConstant(d); }) | to_vector;
        return at->CreateExpression(move(values));
    }

    Expression* ExpressionVisitor::Visit(Parser::TupleExpression* arg)
    {
        auto exprs = VisitRange(arg->Expressions, this);
        auto types = exprs | vs::transform([](auto expr) { return expr->Type()->RemoveReference(); }) | to_vector;
        return ctx->GetTupleType(move(types))->CreateExpression(move(exprs));
    }

    Expression* ExpressionVisitor::Visit(Parser::ParenthisedExpression* arg)
    {
        return Dispatch(arg->UnderlyingExpression, this);
    }

    Expression* ExpressionVisitor::Visit(Parser::BinaryOperatorExpression* arg)
    {
        auto left = Dispatch(arg->Left, this);
        auto right = Dispatch(arg->Right, this);
        auto fn = ctx->FindFunction("operator{}"_f(arg->Operation->Text()));
        auto overs = fn->FindCallable({ left->Type(), right->Type() });
        assert(!overs.empty());
        auto type = overs[0]->ReturnType();
        return ctx->CreateObject<BinaryExpression>(type, left, arg->Operation, right);
    }

    Expression* ExpressionVisitor::Visit(Parser::UnaryOperatorExpression* arg)
    {
        auto v = Dispatch(arg->Operand, this);
        assert(v);
        auto ty = v->Type()->RemoveReference();
        auto f = ty->FindMethod("operator{}"_f(arg->Operation->Text()));
        assert(f);
        auto overs = f->FindCallable({ v->Type()->ReferenceTo() });
        assert(!overs.empty());
        return ctx->CreateObject<UnaryExpression>(overs[0]->ReturnType(), arg->Operation, v);
    }

    Expression* ExpressionVisitor::Visit(Parser::IdentifierExpression* arg)
    {
        if (arg->Identifier == "ExternC")
            return ctx->CreateObject<ExternCAttribute>();
        if (auto f = ctx->FindFunction(arg->Identifier))
            return f;
        if (auto v = ctx->FindVariable(arg->Identifier))
            return v->Reference();
        return nullptr;
    }

    Expression* ExpressionVisitor::Visit(Parser::CallExpression* arg)
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
        auto type = Dispatch<Type*>(callee, [&](Function* fn)
        {
            auto types = args | type_of_range | to_vector;
            auto overloads = fn->FindCallable(types);
            assert(overloads.size() == 1);
            return overloads.front()->ReturnType();
        });
        return ctx->CreateObject<CallExpression>(type, callee, move(args));
    }

    Expression* ExpressionVisitor::Visit(Parser::TernaryExpression* arg)
    {
        auto cond  = Dispatch(arg->Condition, this);
        auto texpr = Dispatch(arg->TrueExpression, this);
        auto fexpr = Dispatch(arg->FalseExpression, this);
        return ctx->CreateObject<TernaryExpression>(texpr->Type(), cond, texpr, fexpr);
    }

    Expression* ExpressionVisitor::Visit(Parser::NewExpression* arg)
    {
        auto type = Dispatch(arg->Type, typeVisitor)->PointerTo();
        auto args = VisitRange(arg->Arguments, this);
        return ctx->CreateObject<NewExpression>(type, move(args));
    }

    Expression* ExpressionVisitor::Visit(Parser::NamedOperatorExpression* arg)
    {
        switch (arg->Token->KindId())
        {
        case Delete::Id: return ctx->CreateObject<DeleteExpression>(Dispatch(arg->Argument, this));
        case Return::Id: return ctx->CreateObject<ReturnExpression>(Dispatch(arg->Argument, this));
        default: throw std::runtime_error("Not Implemented!");
        }
    }

    Expression* ExpressionVisitor::Visit(Parser::NamedTypeOperatorExpression* arg)
    {
        auto isize = ctx->GetIntegerType(sizeof(size_t) * 8, true);
        switch (arg->Token->KindId())
        {
        case SizeOf::Id: return ctx->CreateObject<SizeOfExpression>(isize, Dispatch(arg->Type, typeVisitor));
        case AlignOf::Id: return ctx->CreateObject<AlignOfExpression>(isize, Dispatch(arg->Type, typeVisitor));
        default: throw std::runtime_error("Not Implemented!");
        }
    }

    Expression* ExpressionVisitor::Visit(Parser::SubscriptOperator* arg)
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

    Expression* ExpressionVisitor::Visit(Parser::MemberAccessExpression* arg)
    {
        auto owner = Dispatch(arg->Owner, this);
        auto ty = owner->Type()->RemoveReference();
        auto field = ty->FindField(arg->Member->Identifier);
        if (!field && ty->Is<PointerType>())
            field = ty->RemovePointer()->FindField(arg->Member->Identifier);
        if (!field)
        {
            auto prop = ty->FindProperty(arg->Member->Identifier);
            assert(prop != nullptr);
            return ctx->CreateObject<PropertyAccessExpression>(owner, prop);
        }
        return ctx->CreateObject<MemberAccessExpression>(owner, field);
    }

    ExpressionVisitor::ExpressionVisitor(Context* ctx) :
        ctx(ctx),
        typeVisitor(ctx, this)
    {
        
    }

    Statement* StatementVisitor::Visit(Parser::VariableDeclaration* arg)
    {
        auto type = Dispatch(arg->Type, this);
        auto expr = Dispatch(arg->InitializingExpression, this);
        assert(type || expr);
        if (!type)
            type = expr->Type()->RemoveReference();
        return ctx->CreateChild<VariableDeclaration>(arg->Name, type, expr);
    }

    Statement* StatementVisitor::Visit(Parser::TupleVariableDeclaration* arg)
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

    Statement* StatementVisitor::Visit(Parser::BlockStatement* arg)
    {
        auto bs = ctx->CreateChild<BlockStatement>();
        auto vis = StatementVisitor(bs);
        for (auto s : arg->Statements)
            bs->Statements.emplace_back(Dispatch(s, vis));
        return bs;
    }

    Statement* StatementVisitor::Visit(Parser::IfStatement* arg)
    {
        auto ifs = ctx->CreateChild<IfStatement>();
        auto vis = StatementVisitor(ifs);
        ifs->Condition = Dispatch(arg->Condition, vis);
        ifs->TrueStatement = Dispatch(arg->TrueStatement, vis);
        ifs->FalseStatement = Dispatch(arg->FalseStatement, vis);
        return ifs;
    }

    Statement* StatementVisitor::Visit(Parser::ExpressionStatement* arg)
    {
        auto es = ctx->CreateChild<ExpressionStatement>();
        es->UnderlyingExpression = Dispatch(arg->UnderlyingExpression, StatementVisitor(es));
        return es;
    }

    Statement* StatementVisitor::Visit(Parser::WhileStatement* arg)
    {
        auto ws = ctx->CreateChild<WhileStatement>();
        auto vis = StatementVisitor(ws);
        ws->Condition = Dispatch(arg->Condition, vis);
        ws->LoopStatement = Dispatch(arg->LoopStatement, vis);
        return ws;
    }

    Statement* StatementVisitor::Visit(Parser::ForStatement* arg)
    {
        auto fs = ctx->CreateChild<ForStatement>();
        auto vis = StatementVisitor(fs);
        fs->StartStatement  = Dispatch(arg->StartStatement, vis);
        fs->Condition       = Dispatch(arg->Condition, vis);
        fs->Increment       = Dispatch(arg->Increment, vis);
        fs->LoopStatement   = Dispatch(arg->LoopStatement, vis);
        return fs;
    }

    Statement* StatementVisitor::Visit(Parser::SwitchStatement* arg)
    {
        auto ss = ctx->CreateChild<SwitchStatement>();
        auto vis = StatementVisitor(ss);
        ss->Condition = Dispatch(arg->Condition, vis);
        ss->Cases = arg->Cases | vs::transform([&](auto& p) { return std::make_pair(Dispatch(p.first, vis), Dispatch(p.second, vis)); }) | to_vector;
        return ss;
    }

    StatementVisitor::StatementVisitor(Context* ctx) :
        ExpressionVisitor(ctx),
        TypeVisitor(ctx, this),
        ctx(ctx)
    {
        
    }

    Statement* ClassVisitor::Visit(Parser::FunctionDeclaration* arg)
    {
        return nullptr;
    }

    Statement* ClassVisitor::Visit(Parser::PropertyDeclaration* arg)
    {
        auto ct = decl->DeclaredType();
        auto prop = ct->FindProperty(arg->Name);

        auto getFn = prop->Getter;

        auto es = arg->Getter->As<Parser::ExpressionStatement>();
        assert(es);

        auto vis = StatementVisitor(getFn);
        auto expr = Dispatch(es->UnderlyingExpression, vis);
        auto st = getFn->CreateChild<ExpressionStatement>();
        st->UnderlyingExpression = getFn->CreateObject<ReturnExpression>(expr);
        getFn->AddStatement(st);

        return prop;
    }

    Statement* ClassVisitor::Visit(Parser::ClassDeclaration* arg)
    {
        println();
        return nullptr;
    }

    ClassVisitor::ClassVisitor(ClassDeclaration* decl) : StatementVisitor(decl), decl(decl)
    {

    }

    void ModuleVisitor::Visit(Parser::FunctionDeclaration* arg)
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

    void ModuleVisitor::Visit(Parser::ClassDeclaration* arg)
    {
        auto c = mod->FindType(arg->Name)->As<ClassType>();
        auto vis = ClassVisitor(c->Declaration());
        for (auto statement : arg->Statements)
        {
            println("{}", statement->NameOf());
            Dispatch(statement, vis);
        }
    }

    void ModuleVisitor::Visit(Parser::VariableDeclaration* arg)
    {
        auto v = mod->FindVariable(arg->Name);
        v->AddInitializingExpression(Dispatch(arg->InitializingExpression, this));
    }

    void ModuleVisitor::Visit(Parser::Module* parseTree)
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
                    },
                    [&](Parser::PropertyDeclaration* prop)
                    {
                        auto ty = Dispatch(prop->Type, typeVisitor);
                        auto pd = type->DeclareProperty(prop->Name, ty);
                        assert(prop->Getter != nullptr);
                        pd->Getter = type->DeclareMethod("$get.{}"_f(prop->Name), {  }, ty);
                        if (prop->Setter)
                            pd->Setter = type->DeclareMethod("$set.{}"_f(prop->Name), { pair("value", ty->ReferenceTo()) }, ty); //TODO change to const reference.
                    }
                );
            }
        );

        return VisitRange(parseTree->Statements, this);
    }

    ModuleVisitor::ModuleVisitor(Module* mod) :
        ExpressionVisitor(mod),
        TypeVisitor(mod, this),
        mod(mod)
    {
        
    }

    Module* CreateAst(SystemModule* sys, Parser::Module* parseTree)
    {
        auto mod = sys->CreateObject<Module>(parseTree->Name);
        ModuleVisitor(mod).Visit(parseTree);
        return mod;
    }
    
}
