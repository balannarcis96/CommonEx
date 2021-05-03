#pragma once

namespace std
{
	//SFINAE util that checks if T is a fuctor with operator()'s signature == TSignature
	template<typename T, typename TSignature>
	struct is_functor
	{
		template<typename TType, TType> struct is_same_type;
		typedef char TrueType[1];
		typedef char FalseType[2];

		template<typename TTarget> static TrueType& check(is_same_type<TSignature, &TTarget::operator()>*);
		template<typename	     > static FalseType& check(...);

		static bool const value = sizeof(check<T>(nullptr)) == sizeof(TrueType);
	};

	//Most reliable way (i found + edit) to check if T is a lambda with a specific signature
	namespace lambda_ex
	{
		template <typename T>
		struct identity { using type = T; };

		template <typename...>
		using void_t = void;

		template <typename F>
		struct call_operator;

		template <typename C, typename R, typename... A>
		struct call_operator<R(C::*)(A...)> : identity<R(A...)> {};

		template <typename C, typename R, typename... A>
		struct call_operator<R(C::*)(A...) const> : identity<R(A...)> {};

		template <typename C, typename R, typename... A>
		struct call_operator<R(C::*)(A...) noexcept> : identity<R(A...)> {};

		template <typename C, typename R, typename... A>
		struct call_operator<R(C::*)(A...) const noexcept> : identity<R(A...)> {};

		template <typename F>
		using call_operator_t = typename call_operator<F>::type;

		template <typename, typename, typename = void_t<>>
		struct is_convertible_to_function
			: std::false_type
		{};

		template <typename L, typename TSignature>
		struct is_convertible_to_function<L, TSignature, void_t<decltype(&L::operator())>>
			: std::is_assignable<call_operator_t<decltype(&L::operator())>*&, TSignature>
		{};
	}

	template <typename L, typename TSignature>
	using is_lambda = lambda_ex::is_convertible_to_function<L, TSignature>;

	template <typename L, typename TReturnType, typename ... TArgs>
	using is_lambda_f = lambda_ex::is_convertible_to_function<L, TReturnType(TArgs...)>;

	//type_if , use T1 of T2 based on a compile time test 
	template<bool Test, typename T1, typename T2>
	struct type_if;

	template<typename T1, typename T2>
	struct type_if<true, T1, T2>
	{
		using type = T1;
	};

	template<typename T1, typename T2>
	struct type_if <false, T1, T2>
	{
		using type = T2;
	};

	template<bool Test, typename T1, typename T2>
	using type_if_t = typename type_if<Test, T1, T2>::type;
}
