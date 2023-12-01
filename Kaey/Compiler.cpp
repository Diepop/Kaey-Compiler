#include "Compiler.hpp"

#include "Llvm/Function.hpp"
#include "Llvm/ModuleContext.hpp"
#include "Llvm/Types.hpp"

#include "Ast/Expressions.hpp"
#include "Ast/Statements.hpp"
#include "Ast/Types.hpp"
#include "Ast/Module.hpp"

namespace Kaey::Llvm
{
    namespace
    {
        using ClassMap = unordered_map<Ast::ClassType*, ClassType*>;

        struct TypeVisitor : Ast::Type::Visitor<Type*>
        {
            VoidType* Visit(Ast::VoidType*) { return mod->GetVoidType(); }

            BooleanType* Visit(Ast::BooleanType*) { return mod->GetBooleanType(); }

            CharacterType* Visit(Ast::CharacterType*) { return mod->GetCharacterType(); }

            IntegerType* Visit(Ast::IntegerType* type) { return mod->GetIntegerType(type->Bits(), type->IsSigned()); }

            FloatType* Visit(Ast::FloatType*) { return mod->GetFloatType(); }

            PointerType* Visit(Ast::PointerType* type) { return Dispatch(type->UnderlyingType(), this)->PointerTo(); }

            ReferenceType* Visit(Ast::ReferenceType* type) { return Dispatch(type->UnderlyingType(), this)->ReferenceTo(); }

            ArrayType* Visit(Ast::ArrayType* type) { return mod->GetArrayType(Dispatch(type->UnderlyingType(), this), type->Length()); }

            TupleType* Visit(Ast::TupleType* type) { return mod->GetTupleType(VisitRange(type->ElementTypes(), this)); }

            FunctionPointerType* Visit(Ast::FunctionPointerType* type) { return mod->GetFunctionPointerType(Dispatch(type->ReturnType(), this), VisitRange(type->ParameterTypes(), this), type->IsVariadic()); }

            VariantType* Visit(Ast::VariantType* type) { return mod->GetVariantType(VisitRange(type->UnderlyingTypes(), this)); }

            ClassType* Visit(Ast::ClassType* type) { return classes->at(type); }

            OptionalType* Visit(Ast::OptionalType* type) { return Dispatch(type->UnderlyingType(), this)->OptionalTo(); }

            TypeVisitor(ModuleContext* mod, ClassMap* classes) : mod(mod), classes(classes) {  }

        private:
            ModuleContext* mod;
            ClassMap* classes;

        };

        struct FunctionVisitor : Ast::Statement::Visitor<void>, Ast::Expression::Visitor<Expression*>
        {
            void Visit(Ast::ExpressionStatement* statement)
            {
                Dispatch(statement->UnderlyingExpression, this);
            }

            void Visit(Ast::VariableDeclaration* variable)
            {
                function->AddOption((string)variable->Name());
                if (auto ie = variable->InitializingExpression())
                {
                    if (!ie->IsConstant())
                        varMap.emplace(variable->Name(), Dispatch(ie, this));
                    else
                    {
                        auto type = Dispatch(variable->Type(), typeVisitor);
                        function->BeginCall(type);
                            function->AddCallParameter(Dispatch(ie, this));
                        varMap.emplace(variable->Name(), function->EndCall());
                    }
                }
                else varMap.emplace(variable->Name(), function->CallUnscoped(Dispatch(variable->Type(), typeVisitor)));
            }

            void Visit(Ast::TupleVariableDeclaration* statement)
            {
                auto ie = statement->InitializeExpression() ? Dispatch(statement->InitializeExpression(), this) : nullptr;
                auto vars = statement->Variables();
                for (size_t i = 0; i < vars.size(); ++i)
                {
                    auto n = vars[i]->Name();
                    auto ty = Dispatch(statement->Type()->ElementTypes()[i], typeVisitor);
                    if (ie)
                    {
                        auto v = mod->CreateObject<VariableReference>(function, (string)n, ty->ReferenceTo(), function->GetField(ie, i)->Value());
                        varMap.emplace(n, v);
                    }
                }
            }

            void Visit(Ast::BlockStatement* statement)
            {
                function->Begin();
                VisitRange(statement->Statements, this);
                function->End();
            }

            void Visit(Ast::IfStatement* statement)
            {
                function->BeginIf();
                function->AddIfCondition(Dispatch(statement->Condition, this));
                Dispatch(statement->TrueStatement, this);
                if (auto fs = statement->FalseStatement)
                {
                    function->BeginElse();
                    Dispatch(fs, this);
                }
                function->EndIf();
            }

            void Visit(Ast::WhileStatement* statement)
            {
                function->BeginWhile();
                function->AddWhileCondition(Dispatch(statement->Condition, this));
                Dispatch(statement->LoopStatement, this);
                function->EndWhile();
            }

            void Visit(Ast::ForStatement* statement)
            {
                function->Begin();
                Dispatch(statement->StartStatement, this);
                function->BeginWhile();
                function->AddWhileCondition(Dispatch(statement->Condition, this));
                function->Begin();
                Dispatch(statement->LoopStatement, this);
                function->End();
                Dispatch(statement->Increment, this);
                function->EndWhile();
                function->End();
            }

            void Visit(Ast::SwitchStatement* statement)
            {
                function->BeginSwitch();
                function->AddSwitchCondition(Dispatch(statement->Condition, this));
                for (auto& [e, s] : statement->Cases)
                {
                    function->BeginCase();
                    auto ee = e ? Dispatch(e, this)->As<Constant>() : nullptr;
                    assert(!e || ee);
                    function->AddCaseValue(ee);
                    if (auto bs = s->As<Ast::BlockStatement>())
                        VisitRange(bs->Statements, this);
                    else Dispatch(s, this);
                    function->EndCase();
                }
                function->EndSwitch();
            }

            Expression* Visit(Ast::BinaryExpression* expr)
            {
                auto op = "operator{}"_f(expr->Operator()->Text());
                function->BeginCall(op);
                    function->AddCallParameter(Dispatch(expr->LeftOperand(), this));
                    function->AddCallParameter(Dispatch(expr->RightOperand(), this));
                return function->EndCall();
            }

            Expression* Visit(Ast::UnaryExpression* expr)
            {
                auto op = "operator{}"_f(expr->Operator()->Text());
                function->BeginCall(op);
                    function->AddCallParameter(Dispatch(expr->Operand(), this));
                return function->EndCall();
            }

            Expression* Visit(Ast::SubscriptOperatorExpression* call)
            {
                function->BeginCall(SUBSCRIPT_OPERATOR);
                function->AddCallParameter(Dispatch(call->Callee(), this));
                for (auto arg : call->Arguments())
                    function->AddCallParameter(Dispatch(arg, this));
                return function->EndCall();
            }

            Expression* Visit(Ast::CallExpression* call)
            {
                auto callee = call->Callee()->As<Ast::Function>();
                assert(callee);
                auto fn = mod->FindFunction(callee->Name());
                function->BeginCall(fn);
                for (auto arg : call->Arguments())
                    function->AddCallParameter(Dispatch(arg, this));
                return function->EndCall();
            }

            Expression* Visit(Ast::VariableReferenceExpression* v)
            {
                auto it = varMap.find(v->Declaration()->Name());
                assert(it != varMap.end());
                return it->second;
            }

            Expression* Visit(Ast::BooleanConstant* expr)
            {
                auto type = mod->GetBooleanType();
                return function->AddExpression(type->CreateConstant(expr->Value()));
            }

            Expression* Visit(Ast::IntegerConstant* expr)
            {
                auto type = typeVisitor->Visit(expr->TypeOfInteger());
                return function->AddExpression(type->CreateConstantUnsigned(expr->Value()));
            }

            Expression* Visit(Ast::FloatConstant* expr)
            {
                auto type = typeVisitor->Visit(expr->TypeOfFloat());
                return function->AddExpression(type->CreateConstant(expr->Value()));
            }

            Expression* Visit(Ast::CharacterConstant* expr)
            {
                auto type = mod->GetCharacterType();
                return function->AddExpression(type->CreateConstant(expr->Value()));
            }

            Expression* Visit(Ast::ArrayExpression* expr)
            {
                auto type = typeVisitor->Visit(expr->TypeOfArray());
                auto v = VisitRange(expr->Values(), this) | Cast<Constant>() | to_vector;
                return function->AddExpression(type->CreateConstant(move(v)));
            }

            Expression* Visit(Ast::NewExpression* expr)
            {
                auto type = Dispatch(expr->Type()->RemovePointer(), typeVisitor);
                auto ptr = function->Malloc(type);
                function->BeginCall(STAR_OPERATOR);
                    function->AddCallParameter(ptr);
                type->Construct(function, function->EndCall(), VisitRange(expr->Arguments(), this));
                return ptr;
            }

            Expression* Visit(Ast::DeleteExpression* expr)
            {
                auto ptr = Dispatch(expr->Argument(), this);
                function->BeginCall(STAR_OPERATOR);
                    function->AddCallParameter(ptr);
                function->Destruct(function->EndCall());
                return function->Free(ptr);
            }

            Expression* Visit(Ast::MemberAccessExpression* expr)
            {
                auto owner = Dispatch(expr->Owner(), this);
                return owner->GetField(function, expr->Field()->Name());
            }

            Expression* Visit(Ast::TupleExpression* expr)
            {
                auto ty = typeVisitor->Visit(expr->TypeOfTuple());
                function->BeginCall(ty);
                for (auto e : VisitRange(expr->Values(), this))
                    function->AddCallParameter(e);
                return function->EndCall();
            }

            Expression* Visit(Ast::ReturnExpression* expr)
            {
                if (expr->Argument()->Is<Ast::CallExpression>())
                    function->AddOption(ReturnCall{ });
                function->CreateReturn(Dispatch(expr->Argument(), this));
                return nullptr;
            }

            Expression* Visit(Ast::TernaryExpression* expr)
            {
                //TODO
                return nullptr;
            }

            Expression* Visit(Ast::SizeOfExpression* expr)
            {
                auto arg = Dispatch(expr->Argument(), typeVisitor);
                auto type = Dispatch(expr->Type(), typeVisitor)->As<IntegerType>();
                assert(arg && type);
                return type->CreateConstant(arg->Size());
            }

            Expression* Visit(Ast::AlignOfExpression* expr)
            {
                auto arg = Dispatch(expr->Argument(), typeVisitor);
                auto type = Dispatch(expr->Type(), typeVisitor)->As<IntegerType>();
                assert(arg && type);
                return type->CreateConstant(arg->Alignment());
            }

            void operator()(Ast::FunctionDeclaration* decl, FunctionOverload* fn, TypeVisitor* visitor)
            {
                function = fn;
                declaration = decl;
                mod = function->Module();
                typeVisitor = visitor;
                varMap.clear();

                if (declaration->Statements().empty())
                    return;

                function->Begin();
                for (auto param : function->Parameters())
                    varMap.emplace(param->Name(), param);
                /*for (auto sta : declaration->Statements())
                    Dispatch(sta,
                        [&](Ast::FieldInitializer* init)
                        {
                            auto t = init->Reference()->Declaration()->Type()->Visit(typeVisitor);
                            auto types = typeVisitor->DispatchRange(init->Arguments() | type_of_range);
                            auto inst = function->Parameter(0)->GetField(function, init->Reference()->Name());
                            function->AddOption(inst);
                            function->BeginCall(t);
                            for (auto arg : init->Arguments())
                                function->AddCallParameter(Dispatch(arg));
                            function->EndCall();
                        },
                        [&](Ast::ConstructorDelegation* cd)
                        {
                            auto thisptr = function->Parameter(0);
                            auto t = thisptr->Type()->RemoveReference();
                            auto types = typeVisitor->DispatchRange(cd->Arguments() | type_of_range);
                            function->AddOption(thisptr);
                            function->BeginCall(t);
                            for (auto arg : cd->Arguments())
                                function->AddCallParameter(Dispatch(arg));
                            function->EndCall();
                        }
                        );*/

                if (declaration->Statements().empty())
                {
                    function->End();
                    return;
                }
                auto statements = declaration->Statements();
                for (auto statement : statements)
                    Dispatch(statement, this);
                function->End();
            }

        protected:
            Ast::FunctionDeclaration* declaration = nullptr;
            FunctionOverload* function = nullptr;
            ModuleContext* mod = nullptr;
            TypeVisitor* typeVisitor = nullptr;
            unordered_map<string_view, Expression*> varMap;
        };

        struct MethodVisitor : FunctionVisitor
        {
            MethodVisitor(ClassType* type) : type(type) {  }

            void operator()(Ast::ConstructorDeclaration* decl, FunctionOverload* fn, TypeVisitor* visitor)
            {
                function = fn;
                declaration = decl;
                mod = function->Module();
                typeVisitor = visitor;
                varMap.clear();

                if (declaration->Statements().empty() &&
                    decl->DelegateInitialiers().empty() &&
                    decl->FieldInitialiers().empty())
                    return;

                function->Begin();
                for (auto param : function->Parameters())
                    varMap.emplace(param->Name(), param);

                auto th = fn->Parameter(0);
                auto ty = th->Type()->RemoveReference();

                for (auto di : decl->DelegateInitialiers())
                {
                    //TODO fix properly initialization of bases.
                    ty->Construct(fn, fn->Parameter(0), VisitRange(di->Arguments(), this));
                }

                for (auto fi : decl->FieldInitialiers())
                {
                    auto fd = ty->FindField(fi->Field()->Name());
                    auto field = th->GetField(fn, fd->Index);
                    assert(field);
                    auto args = VisitRange(fi->Arguments(), this);
                    fn->Construct(field, move(args));
                }

                for (auto statement : declaration->Statements())
                    Dispatch(statement, this);

                if (declaration->IsDestructor())
                {
                    for (auto fd : ty->Fields() | vs::reverse)
                    {
                        auto field = th->GetField(fn, fd->Index);
                        assert(field);
                        fn->Destruct(field);
                    }
                }

                function->End();
            }

        private:
            ClassType* type;

        };

        struct ModuleVisitor final : Ast::Statement::Visitor<void>, TypeVisitor
        {
            using TypeVisitor::Visit;

            ModuleVisitor(ModuleContext* mod) : TypeVisitor(mod, &classMap), mod(mod) {  }

            void Visit(Ast::Module* ast)
            {
                for (auto decl : ast->Classes()) if (auto ct = decl->DeclaredType())
                    classMap.emplace(ct, mod->DeclareClass((string)ct->Name()));

                for (auto& [ct, type] : classMap)
                {
                    type->BeginFields();
                    for (auto field : ct->Fields())
                        type->DeclareField((string)field->Name(), Dispatch(field->Type(), this));
                    type->EndFields();
                }

                static auto parse_params = vs::transform([&](Ast::ParameterDeclaration* param) -> ArgumentDeclaration
                {
                    return { (string)param->Name(), Dispatch(param->Type(), this) };
                });

                for (auto& [ct, type] : classMap)
                {
                    type->Begin();
                    for (auto constructor : ct->Constructors())
                    {
                        auto params = constructor->Parameters() | vs::drop(1) | parse_params | to_vector;
                        if (constructor->IsConstructor())
                            type->DeclareConstructor(move(params));
                        else type->DeclareDestructor();
                    }
                    for (auto method : ct->Methods()) if (method->Name() != AMPERSAND_OPERATOR)
                    {
                        auto params = method->Parameters() | vs::drop(1) | parse_params | to_vector;
                        type->DeclareMethod(method->Name(), move(params), Dispatch(method->ReturnType(), this));
                    }
                    type->End();
                }

                for (auto function : ast->Functions())
                for (auto overload : function->Overloads())
                {
                    auto params = overload->Parameters() | parse_params | to_vector;
                    bool disableMangling = false;
                    for (auto att : overload->Attributes())
                    {
                        if (rn::any_of(att->Expressions(), [](Ast::Expression* e) { return e->Is<Ast::ExternCAttribute>(); }))
                        {
                            disableMangling = true;
                            break;
                        }
                    }
                    auto decl = mod->DeclareFunction((string)overload->Name(), params, Dispatch(overload->ReturnType(), this), overload->IsVariadic(), disableMangling);
                    fnMap.emplace(overload, decl);
                }

                for (auto variable : ast->Variables())
                    mod->DeclareVariable((string)variable->Name(), Dispatch(variable->Type(), this));

                TypeVisitor typeVisitor{ mod, &classMap };
                FunctionVisitor fnVisitor;

                for (auto& [ast, cl] : classMap)
                {
                    MethodVisitor mVisitor{ cl };
                    for (auto& cc : ast->Constructors())
                    {
                        auto cd = cl->FindMethodOverload(cc->IsConstructor() ? CONSTRUCTOR : DESTRUCTOR, Dispatch(cc->Type(), typeVisitor)->As<FunctionPointerType>()->ParameterTypes() | vs::drop(1) | to_vector)->As<FunctionOverload>();
                        mVisitor(cc, cd, &typeVisitor);
                    }
                    cl->CreateMethods();
                }

                for (auto& [ast, fn] : fnMap)
                    fnVisitor(ast, fn, &typeVisitor);
            }

        private:
            ModuleContext* mod;
            unordered_map<Ast::FunctionDeclaration*, FunctionOverload*> fnMap;
            ClassMap classMap;

        };

    }

    void CompileModule(ModuleContext* mod, Ast::Module* ast)
    {
        ModuleVisitor{ mod }.Visit(ast);
    }

}
