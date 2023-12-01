#pragma once

namespace Kaey::Coroutine
{
    using std::optional;
    using std::suspend_always;
    using std::suspend_never;
    using std::coroutine_handle;
    using std::exception_ptr;

    using std::move;
    using std::forward;
    using std::nullopt;

    template<class Yield>
    struct ResultGeneratorPromise
    {
        exception_ptr* Exception = nullptr;
        optional<Yield>* YieldValue = nullptr;
        ResultGeneratorPromise* Child = nullptr;
        bool Done = false;

        suspend_always initial_suspend() const noexcept { return {}; }

        suspend_never final_suspend() const noexcept { return {}; }

        suspend_always yield_value(Yield& e)
        {
            *YieldValue = move(e);
            return {};
        }

        suspend_always yield_value(Yield&& e) { return yield_value(e); }

        void unhandled_exception() const noexcept
        {
            *Exception = std::current_exception();
        }

        void Resume()
        {
            if (Child)
            {
                Child->Resume();
                if (!Child->Done)
                    return;
            }
            coroutine_handle<ResultGeneratorPromise>::from_promise(*this).resume();
            if (Done)
                return;
            if (Child)
            {
                Child->Resume();
                if (Child->Done)
                {
                    Child = nullptr;
                    Resume();
                }
            }
        }

    };

    template<class Yield, class Result = void>
    struct ResultGenerator
    {
        struct PromiseBase : ResultGeneratorPromise<Yield>
        {
            template<class Res>
            auto await_transform(ResultGenerator<Yield, Res>& generator) noexcept
            {
                struct Awaitable
                {
                    ResultGenerator<Yield, Res>& Child;

                    bool await_ready() const noexcept
                    {
                        return Child;
                    }

                    void await_suspend(coroutine_handle<promise_type> h) noexcept
                    {
                        auto& p = h.promise();
                        p.Child = Child.Promise;
                        Child.Promise->Exception = p.Exception;
                        Child.Promise->YieldValue = p.YieldValue;
                    }

                    Res await_resume()
                    {
                        if constexpr (!std::is_void_v<Res>)
                            return Child.Value();
                        else return;
                    }

                };
                return Awaitable{ generator };
            }

            template<class Res>
            auto await_transform(ResultGenerator<Yield, Res>&& generator) noexcept { return await_transform(generator); }

        };

        template<class Res>
        struct PromiseType : PromiseBase
        {
            ResultGenerator<Yield, Res>* Generator = nullptr;

            ResultGenerator<Yield, Res> get_return_object() noexcept { return { this }; }

            void return_value(Res& v)
            {
                Generator->ResultValue = move(v);
                this->Done = true;
            }

            void return_value(Res&& v)
            {
                return return_value(v);
            }

            template<class Arg>
                requires std::is_trivially_constructible_v<Res, Arg&&>
            void return_value(Arg&& arg)
            {
                return return_value(Res(forward<Arg>(arg)));
            }

        };

        template<>
        struct PromiseType<void> : PromiseBase
        {
            ResultGenerator<Yield, void>* Generator = nullptr;

            ResultGenerator<Yield, void> get_return_object() noexcept { return { this }; }

            void return_void()
            {
                Generator->ResultValue = this->Done = true;
            }

        };

        struct iterator
        {
            using value_type = Yield;
            using reference_type = value_type&;
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;

            ResultGenerator* Generator;
            mutable optional<Yield> YieldValue;

            iterator(ResultGenerator* generator = nullptr) : Generator(generator) {  }

            reference_type operator*() const { return *YieldValue; }

            reference_type operator->() const { return **this; }

            iterator& operator++()
            {
                YieldValue = Generator->ResumeYield();
                return *this;
            }

            iterator operator++(int)
            {
                auto it = *this;
                ++*this;
                return it;
            }

            friend bool operator==(const iterator& lhs, const iterator& rhs)
            {
                return lhs.Generator == rhs.Generator && lhs.YieldValue.has_value() == rhs.YieldValue.has_value();
            }

        };

        using result_type = Result;
        using yield_type = Yield;
        using promise_type = PromiseType<Result>;

        promise_type* Promise;
        exception_ptr Exception;
        optional<Yield> YieldValue;
        std::conditional_t<std::is_void_v<Result>, bool, optional<Result>> ResultValue;

        ResultGenerator(promise_type* promise = nullptr) : Promise(promise), ResultValue()
        {
            if (!promise)
                return;
            promise->Generator = this;
            promise->Exception = &Exception;
            promise->YieldValue = &YieldValue;
        }

        ResultGenerator(ResultGenerator&&) = delete;
        ResultGenerator(const ResultGenerator&) = delete;

        ResultGenerator& operator=(ResultGenerator&&) = delete;
        ResultGenerator& operator=(const ResultGenerator&) = delete;

        ~ResultGenerator() = default;

        bool Resume()
        {
            if (!ResultValue && !YieldValue && !Exception)
                Promise->Resume();
            if (auto e = Exception)
            {
                Exception = nullptr;
                std::rethrow_exception(e);
            }
            return !*this;
        }

        Yield operator()()
        {
            if (!YieldValue)
                Promise->Resume();
            auto res = move(*YieldValue);
            YieldValue.reset();
            return res;
        }

        operator bool() const
        {
            return (bool)ResultValue;
        }

        Result Value() requires !std::is_void_v<Result>
        {
            assert(ResultValue);
            return move(*ResultValue);
        }

        optional<Yield> ResumeYield()
        {
            if (Resume())
                return (*this)();
            return nullopt;
        }

        iterator begin()
        {
            return ++iterator{ this };
        }

        iterator end()
        {
            return { this };
        }

    };

}